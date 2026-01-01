/**
 * @file client.cpp
 * @brief Implementation of the Ascnd API client
 *
 * This file provides the non-header-only implementation of AscndClient.
 * When ASCND_HEADER_ONLY is defined, the implementation in client.hpp is used instead.
 */

#ifndef ASCND_HEADER_ONLY

#include "ascnd/client.hpp"
#include <httplib.h>
#include <thread>
#include <atomic>

namespace ascnd {

class AscndClient::Impl {
public:
    ClientConfig config;
    std::unique_ptr<httplib::Client> http_client;
    mutable std::mutex mutex;

    explicit Impl(ClientConfig cfg) : config(std::move(cfg)) {
        init_client();
    }

    void init_client() {
        // Parse URL to extract host
        std::string url = config.base_url;

        // Remove trailing slash
        while (!url.empty() && url.back() == '/') {
            url.pop_back();
        }

        http_client = std::make_unique<httplib::Client>(url);

        // Configure timeouts
        http_client->set_connection_timeout(
            std::chrono::milliseconds(config.connection_timeout_ms)
        );
        http_client->set_read_timeout(
            std::chrono::milliseconds(config.read_timeout_ms)
        );
        http_client->set_write_timeout(
            std::chrono::milliseconds(config.write_timeout_ms)
        );

        // Set default headers
        httplib::Headers headers;
        headers.emplace("Content-Type", "application/json");
        headers.emplace("Accept", "application/json");

        if (!config.api_key.empty()) {
            headers.emplace("Authorization", "Bearer " + config.api_key);
        }

        if (!config.user_agent.empty()) {
            headers.emplace("User-Agent", config.user_agent);
        } else {
            headers.emplace("User-Agent", "ascnd-cpp-client/1.0.0");
        }

        http_client->set_default_headers(headers);
    }

    template<typename ResponseT>
    Result<ResponseT> make_request(
        const std::string& method,
        const std::string& path,
        const json& body = json{}
    ) {
        std::lock_guard<std::mutex> lock(mutex);

        httplib::Result result;
        std::string body_str = body.empty() ? "" : body.dump();

        int retries = 0;
        while (retries <= config.max_retries) {
            if (method == "POST") {
                result = http_client->Post(path, body_str, "application/json");
            } else if (method == "GET") {
                result = http_client->Get(path);
            } else {
                return Result<ResponseT>::error("Unsupported HTTP method: " + method);
            }

            if (result) {
                break;
            }

            // Retry on connection errors
            if (retries < config.max_retries) {
                int delay = config.retry_delay_ms * (1 << retries);
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
            ++retries;
        }

        if (!result) {
            return Result<ResponseT>::error(
                "HTTP request failed: " + httplib::to_string(result.error()),
                static_cast<int>(result.error())
            );
        }

        auto& response = result.value();

        if (response.status >= 200 && response.status < 300) {
            try {
                auto json_response = json::parse(response.body);
                return Result<ResponseT>::ok(json_response.get<ResponseT>());
            } catch (const json::exception& e) {
                return Result<ResponseT>::error(
                    std::string("JSON parse error: ") + e.what(),
                    response.status
                );
            }
        }

        // Handle error response
        std::string error_msg;
        try {
            auto error_json = json::parse(response.body);
            if (error_json.contains("message")) {
                error_msg = error_json["message"].get<std::string>();
            } else if (error_json.contains("error")) {
                error_msg = error_json["error"].get<std::string>();
            } else {
                error_msg = response.body;
            }
        } catch (...) {
            error_msg = response.body.empty() ?
                "HTTP " + std::to_string(response.status) : response.body;
        }

        return Result<ResponseT>::error(error_msg, response.status);
    }
};

AscndClient::AscndClient(ClientConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

AscndClient::AscndClient(const std::string& base_url, const std::string& api_key)
    : impl_(std::make_unique<Impl>(ClientConfig{base_url, api_key})) {}

AscndClient::~AscndClient() = default;

AscndClient::AscndClient(AscndClient&&) noexcept = default;
AscndClient& AscndClient::operator=(AscndClient&&) noexcept = default;

Result<SubmitScoreResponse> AscndClient::submit_score(const SubmitScoreRequest& request) {
    json body = request;
    return impl_->make_request<SubmitScoreResponse>("POST", "/v1/scores", body);
}

Result<GetLeaderboardResponse> AscndClient::get_leaderboard(const GetLeaderboardRequest& request) {
    std::string path = "/v1/leaderboards/" + request.leaderboard_id;
    std::string query;

    if (request.limit.has_value()) {
        query += (query.empty() ? "?" : "&");
        query += "limit=" + std::to_string(request.limit.value());
    }
    if (request.offset.has_value()) {
        query += (query.empty() ? "?" : "&");
        query += "offset=" + std::to_string(request.offset.value());
    }
    if (request.period.has_value()) {
        query += (query.empty() ? "?" : "&");
        query += "period=" + request.period.value();
    }

    return impl_->make_request<GetLeaderboardResponse>("GET", path + query);
}

Result<GetPlayerRankResponse> AscndClient::get_player_rank(const GetPlayerRankRequest& request) {
    std::string path = "/v1/leaderboards/" + request.leaderboard_id +
                       "/players/" + request.player_id;

    if (request.period.has_value()) {
        path += "?period=" + request.period.value();
    }

    return impl_->make_request<GetPlayerRankResponse>("GET", path);
}

Result<SubmitScoreResponse> AscndClient::submit_score(
    const std::string& leaderboard_id,
    const std::string& player_id,
    int64_t score
) {
    SubmitScoreRequest request;
    request.leaderboard_id = leaderboard_id;
    request.player_id = player_id;
    request.score = score;
    return submit_score(request);
}

Result<GetLeaderboardResponse> AscndClient::get_leaderboard(
    const std::string& leaderboard_id,
    int32_t limit
) {
    GetLeaderboardRequest request;
    request.leaderboard_id = leaderboard_id;
    request.limit = limit;
    return get_leaderboard(request);
}

Result<GetPlayerRankResponse> AscndClient::get_player_rank(
    const std::string& leaderboard_id,
    const std::string& player_id
) {
    GetPlayerRankRequest request;
    request.leaderboard_id = leaderboard_id;
    request.player_id = player_id;
    return get_player_rank(request);
}

void AscndClient::submit_score_async(
    const SubmitScoreRequest& request,
    AsyncCallback<SubmitScoreResponse> callback
) {
    std::thread([this, request, callback = std::move(callback)]() {
        auto result = submit_score(request);
        callback(std::move(result));
    }).detach();
}

void AscndClient::get_leaderboard_async(
    const GetLeaderboardRequest& request,
    AsyncCallback<GetLeaderboardResponse> callback
) {
    std::thread([this, request, callback = std::move(callback)]() {
        auto result = get_leaderboard(request);
        callback(std::move(result));
    }).detach();
}

void AscndClient::get_player_rank_async(
    const GetPlayerRankRequest& request,
    AsyncCallback<GetPlayerRankResponse> callback
) {
    std::thread([this, request, callback = std::move(callback)]() {
        auto result = get_player_rank(request);
        callback(std::move(result));
    }).detach();
}

void AscndClient::set_api_key(const std::string& api_key) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config.api_key = api_key;
    impl_->init_client();
}

const ClientConfig& AscndClient::config() const noexcept {
    return impl_->config;
}

bool AscndClient::ping() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto result = impl_->http_client->Get("/health");
    return result && result->status >= 200 && result->status < 300;
}

} // namespace ascnd

#endif // ASCND_HEADER_ONLY

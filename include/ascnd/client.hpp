#pragma once

/**
 * @file client.hpp
 * @brief Ascnd API client for C++ games
 *
 * Thread-safe client for interacting with the Ascnd leaderboard API.
 * Designed for use in game engines like Unreal Engine and custom game projects.
 */

#include "types.hpp"

#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>

// Forward declare httplib types to avoid header pollution
namespace httplib {
    class Client;
}

namespace ascnd {

/**
 * @brief Configuration options for AscndClient
 */
struct ClientConfig {
    /// Base URL of the Ascnd API (e.g., "https://api.ascnd.gg")
    std::string base_url;

    /// API key for authentication
    std::string api_key;

    /// Connection timeout in milliseconds (default: 5000)
    int connection_timeout_ms = 5000;

    /// Read timeout in milliseconds (default: 10000)
    int read_timeout_ms = 10000;

    /// Write timeout in milliseconds (default: 10000)
    int write_timeout_ms = 10000;

    /// Number of retry attempts on transient failures (default: 3)
    int max_retries = 3;

    /// Base delay between retries in milliseconds (exponential backoff)
    int retry_delay_ms = 100;

    /// Custom User-Agent string (optional)
    std::string user_agent;

    /// Enable verbose logging (default: false)
    bool verbose = false;
};

/**
 * @brief Callback type for async operations
 */
template<typename T>
using AsyncCallback = std::function<void(Result<T>)>;

/**
 * @brief Thread-safe client for the Ascnd leaderboard API
 *
 * This client provides both synchronous and asynchronous methods
 * for interacting with the Ascnd API. All methods are thread-safe.
 *
 * Example usage:
 * @code
 * ascnd::ClientConfig config;
 * config.base_url = "https://api.ascnd.gg";
 * config.api_key = "your-api-key";
 *
 * ascnd::AscndClient client(config);
 *
 * // Submit a score
 * ascnd::SubmitScoreRequest req;
 * req.leaderboard_id = "high-scores";
 * req.player_id = "player123";
 * req.score = 1000;
 *
 * auto result = client.submit_score(req);
 * if (result.is_ok()) {
 *     std::cout << "Rank: " << result.value().rank << std::endl;
 * }
 * @endcode
 */
class AscndClient {
public:
    /**
     * @brief Construct a client with the given configuration
     * @param config Client configuration
     */
    explicit AscndClient(ClientConfig config);

    /**
     * @brief Construct a client with base URL and API key
     * @param base_url Base URL of the API
     * @param api_key API key for authentication
     */
    AscndClient(const std::string& base_url, const std::string& api_key);

    /// Destructor
    ~AscndClient();

    /// Non-copyable
    AscndClient(const AscndClient&) = delete;
    AscndClient& operator=(const AscndClient&) = delete;

    /// Movable
    AscndClient(AscndClient&&) noexcept;
    AscndClient& operator=(AscndClient&&) noexcept;

    // ========================================================================
    // Synchronous API
    // ========================================================================

    /**
     * @brief Submit a score to a leaderboard
     * @param request Score submission details
     * @return Result containing the submission response or error
     */
    Result<SubmitScoreResponse> submit_score(const SubmitScoreRequest& request);

    /**
     * @brief Get leaderboard entries
     * @param request Leaderboard query parameters
     * @return Result containing the leaderboard entries or error
     */
    Result<GetLeaderboardResponse> get_leaderboard(const GetLeaderboardRequest& request);

    /**
     * @brief Get a specific player's rank
     * @param request Player rank query parameters
     * @return Result containing the player's rank info or error
     */
    Result<GetPlayerRankResponse> get_player_rank(const GetPlayerRankRequest& request);

    // ========================================================================
    // Convenience Methods
    // ========================================================================

    /**
     * @brief Submit a score with minimal parameters
     * @param leaderboard_id Leaderboard identifier
     * @param player_id Player identifier
     * @param score Score value
     * @return Result containing the submission response or error
     */
    Result<SubmitScoreResponse> submit_score(
        const std::string& leaderboard_id,
        const std::string& player_id,
        int64_t score
    );

    /**
     * @brief Get top N entries from a leaderboard
     * @param leaderboard_id Leaderboard identifier
     * @param limit Maximum number of entries (default: 10)
     * @return Result containing the leaderboard entries or error
     */
    Result<GetLeaderboardResponse> get_leaderboard(
        const std::string& leaderboard_id,
        int32_t limit = 10
    );

    /**
     * @brief Get a player's rank on a leaderboard
     * @param leaderboard_id Leaderboard identifier
     * @param player_id Player identifier
     * @return Result containing the player's rank info or error
     */
    Result<GetPlayerRankResponse> get_player_rank(
        const std::string& leaderboard_id,
        const std::string& player_id
    );

    // ========================================================================
    // Asynchronous API
    // ========================================================================

    /**
     * @brief Submit a score asynchronously
     * @param request Score submission details
     * @param callback Callback to invoke with the result
     *
     * @note The callback is invoked on a background thread.
     *       Use appropriate synchronization when updating game state.
     */
    void submit_score_async(
        const SubmitScoreRequest& request,
        AsyncCallback<SubmitScoreResponse> callback
    );

    /**
     * @brief Get leaderboard entries asynchronously
     * @param request Leaderboard query parameters
     * @param callback Callback to invoke with the result
     */
    void get_leaderboard_async(
        const GetLeaderboardRequest& request,
        AsyncCallback<GetLeaderboardResponse> callback
    );

    /**
     * @brief Get a player's rank asynchronously
     * @param request Player rank query parameters
     * @param callback Callback to invoke with the result
     */
    void get_player_rank_async(
        const GetPlayerRankRequest& request,
        AsyncCallback<GetPlayerRankResponse> callback
    );

    // ========================================================================
    // Configuration
    // ========================================================================

    /**
     * @brief Update the API key
     * @param api_key New API key
     */
    void set_api_key(const std::string& api_key);

    /**
     * @brief Get the current configuration
     * @return Current client configuration
     */
    [[nodiscard]] const ClientConfig& config() const noexcept;

    /**
     * @brief Test connectivity to the API
     * @return true if the API is reachable
     */
    [[nodiscard]] bool ping();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Header-Only Implementation
// ============================================================================

#ifdef ASCND_HEADER_ONLY
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

inline AscndClient::AscndClient(ClientConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

inline AscndClient::AscndClient(const std::string& base_url, const std::string& api_key)
    : impl_(std::make_unique<Impl>(ClientConfig{base_url, api_key})) {}

inline AscndClient::~AscndClient() = default;

inline AscndClient::AscndClient(AscndClient&&) noexcept = default;
inline AscndClient& AscndClient::operator=(AscndClient&&) noexcept = default;

inline Result<SubmitScoreResponse> AscndClient::submit_score(const SubmitScoreRequest& request) {
    json body = request;
    return impl_->make_request<SubmitScoreResponse>("POST", "/v1/scores", body);
}

inline Result<GetLeaderboardResponse> AscndClient::get_leaderboard(const GetLeaderboardRequest& request) {
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

inline Result<GetPlayerRankResponse> AscndClient::get_player_rank(const GetPlayerRankRequest& request) {
    std::string path = "/v1/leaderboards/" + request.leaderboard_id +
                       "/players/" + request.player_id;

    if (request.period.has_value()) {
        path += "?period=" + request.period.value();
    }

    return impl_->make_request<GetPlayerRankResponse>("GET", path);
}

inline Result<SubmitScoreResponse> AscndClient::submit_score(
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

inline Result<GetLeaderboardResponse> AscndClient::get_leaderboard(
    const std::string& leaderboard_id,
    int32_t limit
) {
    GetLeaderboardRequest request;
    request.leaderboard_id = leaderboard_id;
    request.limit = limit;
    return get_leaderboard(request);
}

inline Result<GetPlayerRankResponse> AscndClient::get_player_rank(
    const std::string& leaderboard_id,
    const std::string& player_id
) {
    GetPlayerRankRequest request;
    request.leaderboard_id = leaderboard_id;
    request.player_id = player_id;
    return get_player_rank(request);
}

inline void AscndClient::submit_score_async(
    const SubmitScoreRequest& request,
    AsyncCallback<SubmitScoreResponse> callback
) {
    std::thread([this, request, callback = std::move(callback)]() {
        auto result = submit_score(request);
        callback(std::move(result));
    }).detach();
}

inline void AscndClient::get_leaderboard_async(
    const GetLeaderboardRequest& request,
    AsyncCallback<GetLeaderboardResponse> callback
) {
    std::thread([this, request, callback = std::move(callback)]() {
        auto result = get_leaderboard(request);
        callback(std::move(result));
    }).detach();
}

inline void AscndClient::get_player_rank_async(
    const GetPlayerRankRequest& request,
    AsyncCallback<GetPlayerRankResponse> callback
) {
    std::thread([this, request, callback = std::move(callback)]() {
        auto result = get_player_rank(request);
        callback(std::move(result));
    }).detach();
}

inline void AscndClient::set_api_key(const std::string& api_key) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config.api_key = api_key;
    impl_->init_client();
}

inline const ClientConfig& AscndClient::config() const noexcept {
    return impl_->config;
}

inline bool AscndClient::ping() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    auto result = impl_->http_client->Get("/health");
    return result && result->status >= 200 && result->status < 300;
}

} // namespace ascnd

#endif // ASCND_HEADER_ONLY

} // namespace ascnd

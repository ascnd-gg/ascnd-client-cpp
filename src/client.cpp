/**
 * @file client.cpp
 * @brief Implementation of the Ascnd gRPC API client
 */

#include "ascnd/client.hpp"

#include <grpcpp/grpcpp.h>
#include <glog/logging.h>

#include <algorithm>
#include <atomic>
#include <future>
#include <mutex>
#include <thread>

namespace ascnd {

// ============================================================================
// Logging Implementation
// ============================================================================

namespace {

// Thread-safe once flag for logging initialization
std::once_flag g_logging_init_flag;
std::atomic<bool> g_logging_initialized{false};

void DoInitLogging(const LoggingOptions& options) {
    // Initialize GLOG with program name
    google::InitGoogleLogging(options.program_name.c_str());

    // Configure stderr-only logging (no log files)
    FLAGS_logtostderr = true;
    FLAGS_alsologtostderr = false;
    FLAGS_stderrthreshold = 0;  // Log all levels to stderr

    // Set minimum log level based on options
    switch (options.min_level) {
        case LogLevel::kError:
            FLAGS_minloglevel = google::GLOG_ERROR;
            FLAGS_v = 0;
            break;
        case LogLevel::kWarning:
            FLAGS_minloglevel = google::GLOG_WARNING;
            FLAGS_v = 0;
            break;
        case LogLevel::kInfo:
            FLAGS_minloglevel = google::GLOG_INFO;
            FLAGS_v = 0;
            break;
        case LogLevel::kDebug:
            FLAGS_minloglevel = google::GLOG_INFO;
            FLAGS_v = 1;  // Enable VLOG(1) for debug
            break;
    }

    // Color output (may not work on all terminals)
    FLAGS_colorlogtostderr = options.colorize;

    g_logging_initialized = true;
}

void EnsureLoggingInitialized() {
    std::call_once(g_logging_init_flag, []() {
        DoInitLogging(LoggingOptions{});
    });
}

}  // anonymous namespace

void InitLogging(const LoggingOptions& options) {
    std::call_once(g_logging_init_flag, [&options]() {
        DoInitLogging(options);
    });
}

void ShutdownLogging() {
    bool expected = true;
    if (g_logging_initialized.compare_exchange_strong(expected, false)) {
        google::ShutdownGoogleLogging();
    }
}

// ============================================================================
// Client Implementation
// ============================================================================

class AscndClient::Impl {
public:
    ClientConfig config;
    std::shared_ptr<grpc::Channel> channel;
    std::unique_ptr<::ascnd::v1::AscndService::Stub> stub;
    mutable std::mutex mutex;

    // Track pending async operations for proper cleanup
    std::vector<std::future<void>> pending_operations;
    std::mutex pending_mutex;

    explicit Impl(ClientConfig cfg) : config(std::move(cfg)) {
        config.validate();  // Throws std::invalid_argument on invalid config

        // Auto-initialize logging if not already done
        EnsureLoggingInitialized();

        // If verbose mode is enabled, increase VLOG level
        if (config.verbose) {
            FLAGS_v = 1;
            VLOG(1) << "Ascnd client verbose mode enabled";
        }

        LOG(INFO) << "Initializing Ascnd client for " << config.server_address;
        init_channel();
    }

    ~Impl() {
        VLOG(1) << "Shutting down Ascnd client";

        // Wait for all pending async operations to complete
        std::lock_guard<std::mutex> lock(pending_mutex);
        if (!pending_operations.empty()) {
            VLOG(1) << "Waiting for " << pending_operations.size()
                    << " pending async operations";
        }
        for (auto& future : pending_operations) {
            if (future.valid()) {
                future.wait();
            }
        }

        VLOG(1) << "Client shutdown complete";
    }

    // Add a pending operation and clean up completed ones
    void add_pending_operation(std::future<void> op) {
        std::lock_guard<std::mutex> lock(pending_mutex);
        // Clean up completed operations
        pending_operations.erase(
            std::remove_if(pending_operations.begin(), pending_operations.end(),
                [](std::future<void>& f) {
                    return !f.valid() || f.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
                }),
            pending_operations.end()
        );
        // Add new operation
        pending_operations.push_back(std::move(op));
    }

    void init_channel() {
        std::shared_ptr<grpc::ChannelCredentials> creds;

        if (config.use_ssl) {
            VLOG(1) << "Creating SSL channel to " << config.server_address;
            creds = grpc::SslCredentials(grpc::SslCredentialsOptions());
        } else {
            LOG(WARNING) << "Creating insecure channel to " << config.server_address
                         << " (SSL disabled)";
            creds = grpc::InsecureChannelCredentials();
        }

        // Set channel arguments
        grpc::ChannelArguments args;

        if (!config.user_agent.empty()) {
            args.SetUserAgentPrefix(config.user_agent);
        } else {
            args.SetUserAgentPrefix("ascnd-cpp-client/1.0.0");
        }

        // Create channel
        channel = grpc::CreateCustomChannel(config.server_address, creds, args);
        stub = ::ascnd::v1::AscndService::NewStub(channel);

        VLOG(1) << "Channel created successfully";
    }

    std::unique_ptr<grpc::ClientContext> create_context() {
        auto context = std::make_unique<grpc::ClientContext>();

        // Set deadline
        auto deadline = std::chrono::system_clock::now() +
                        std::chrono::milliseconds(config.request_timeout_ms);
        context->set_deadline(deadline);

        // Add API key as metadata
        if (!config.api_key.empty()) {
            context->AddMetadata("authorization", "Bearer " + config.api_key);
        }

        return context;
    }

    template<typename RequestT, typename ResponseT, typename RpcFunc>
    Result<ResponseT> make_request(const RequestT& request, RpcFunc rpc_func) {
        std::lock_guard<std::mutex> lock(mutex);

        VLOG(1) << "Starting request (max retries: " << config.max_retries << ")";

        ResponseT response;
        grpc::Status status;

        int retries = 0;
        while (retries <= config.max_retries) {
            auto context = create_context();
            status = rpc_func(context.get(), request, &response);

            if (status.ok()) {
                VLOG(1) << "Request succeeded on attempt " << (retries + 1);
                return Result<ResponseT>::ok(std::move(response));
            }

            // Only retry on transient errors
            if (!is_retryable_error(status.error_code())) {
                LOG(ERROR) << "Request failed with non-retryable error: "
                           << status.error_message()
                           << " (code: " << static_cast<int>(status.error_code()) << ")";
                break;
            }

            if (retries < config.max_retries) {
                int delay = config.retry_delay_ms * (1 << retries);
                LOG(WARNING) << "Request failed with retryable error: "
                             << status.error_message()
                             << ", retrying in " << delay << "ms"
                             << " (attempt " << (retries + 1) << "/" << config.max_retries << ")";
                std::this_thread::sleep_for(std::chrono::milliseconds(delay));
            }
            ++retries;
        }

        LOG(ERROR) << "Request failed after " << retries << " attempts: "
                   << status.error_message();

        return Result<ResponseT>::error(
            status.error_message(),
            static_cast<int>(status.error_code())
        );
    }

    static bool is_retryable_error(grpc::StatusCode code) {
        switch (code) {
            case grpc::StatusCode::UNAVAILABLE:
            case grpc::StatusCode::DEADLINE_EXCEEDED:
            case grpc::StatusCode::RESOURCE_EXHAUSTED:
            case grpc::StatusCode::ABORTED:
                return true;
            default:
                return false;
        }
    }
};

AscndClient::AscndClient(ClientConfig config)
    : impl_(std::make_unique<Impl>(std::move(config))) {}

AscndClient::AscndClient(const std::string& server_address, const std::string& api_key) {
    ClientConfig config;
    config.server_address = server_address;
    config.api_key = api_key;
    impl_ = std::make_unique<Impl>(std::move(config));
}

AscndClient::~AscndClient() = default;

AscndClient::AscndClient(AscndClient&&) noexcept = default;
AscndClient& AscndClient::operator=(AscndClient&&) noexcept = default;

Result<SubmitScoreResponse> AscndClient::submit_score(const SubmitScoreRequest& request) {
    return impl_->make_request<SubmitScoreRequest, SubmitScoreResponse>(
        request,
        [this](grpc::ClientContext* ctx, const SubmitScoreRequest& req, SubmitScoreResponse* resp) {
            return impl_->stub->SubmitScore(ctx, req, resp);
        }
    );
}

Result<GetLeaderboardResponse> AscndClient::get_leaderboard(const GetLeaderboardRequest& request) {
    return impl_->make_request<GetLeaderboardRequest, GetLeaderboardResponse>(
        request,
        [this](grpc::ClientContext* ctx, const GetLeaderboardRequest& req, GetLeaderboardResponse* resp) {
            return impl_->stub->GetLeaderboard(ctx, req, resp);
        }
    );
}

Result<GetPlayerRankResponse> AscndClient::get_player_rank(const GetPlayerRankRequest& request) {
    return impl_->make_request<GetPlayerRankRequest, GetPlayerRankResponse>(
        request,
        [this](grpc::ClientContext* ctx, const GetPlayerRankRequest& req, GetPlayerRankResponse* resp) {
            return impl_->stub->GetPlayerRank(ctx, req, resp);
        }
    );
}

Result<SubmitScoreResponse> AscndClient::submit_score(
    const std::string& leaderboard_id,
    const std::string& player_id,
    int64_t score
) {
    SubmitScoreRequest request;
    request.set_leaderboard_id(leaderboard_id);
    request.set_player_id(player_id);
    request.set_score(score);
    return submit_score(request);
}

Result<GetLeaderboardResponse> AscndClient::get_leaderboard(
    const std::string& leaderboard_id,
    int32_t limit
) {
    GetLeaderboardRequest request;
    request.set_leaderboard_id(leaderboard_id);
    request.set_limit(limit);
    return get_leaderboard(request);
}

Result<GetPlayerRankResponse> AscndClient::get_player_rank(
    const std::string& leaderboard_id,
    const std::string& player_id
) {
    GetPlayerRankRequest request;
    request.set_leaderboard_id(leaderboard_id);
    request.set_player_id(player_id);
    return get_player_rank(request);
}

// Future-based async methods
std::future<Result<SubmitScoreResponse>> AscndClient::submit_score_async(
    const SubmitScoreRequest& request
) {
    return std::async(std::launch::async, [this, request]() {
        return submit_score(request);
    });
}

std::future<Result<GetLeaderboardResponse>> AscndClient::get_leaderboard_async(
    const GetLeaderboardRequest& request
) {
    return std::async(std::launch::async, [this, request]() {
        return get_leaderboard(request);
    });
}

std::future<Result<GetPlayerRankResponse>> AscndClient::get_player_rank_async(
    const GetPlayerRankRequest& request
) {
    return std::async(std::launch::async, [this, request]() {
        return get_player_rank(request);
    });
}

// Callback-based async methods (tracked for proper lifecycle management)
void AscndClient::submit_score_async(
    const SubmitScoreRequest& request,
    AsyncCallback<SubmitScoreResponse> callback
) {
    auto future = std::async(std::launch::async, [this, request, callback = std::move(callback)]() {
        try {
            auto result = submit_score(request);
            callback(std::move(result));
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception in async callback: " << e.what();
        } catch (...) {
            LOG(ERROR) << "Unknown exception in async callback";
        }
    });
    // Track the operation so destructor waits for completion
    impl_->add_pending_operation(std::move(future));
}

void AscndClient::get_leaderboard_async(
    const GetLeaderboardRequest& request,
    AsyncCallback<GetLeaderboardResponse> callback
) {
    auto future = std::async(std::launch::async, [this, request, callback = std::move(callback)]() {
        try {
            auto result = get_leaderboard(request);
            callback(std::move(result));
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception in async callback: " << e.what();
        } catch (...) {
            LOG(ERROR) << "Unknown exception in async callback";
        }
    });
    impl_->add_pending_operation(std::move(future));
}

void AscndClient::get_player_rank_async(
    const GetPlayerRankRequest& request,
    AsyncCallback<GetPlayerRankResponse> callback
) {
    auto future = std::async(std::launch::async, [this, request, callback = std::move(callback)]() {
        try {
            auto result = get_player_rank(request);
            callback(std::move(result));
        } catch (const std::exception& e) {
            LOG(ERROR) << "Exception in async callback: " << e.what();
        } catch (...) {
            LOG(ERROR) << "Unknown exception in async callback";
        }
    });
    impl_->add_pending_operation(std::move(future));
}

void AscndClient::set_api_key(const std::string& api_key) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config.api_key = api_key;
}

ClientConfig AscndClient::config() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config;
}

bool AscndClient::ping() {
    std::lock_guard<std::mutex> lock(impl_->mutex);

    VLOG(1) << "Testing connection to " << impl_->config.server_address;

    // Wait for channel to be ready with timeout
    auto deadline = std::chrono::system_clock::now() +
                    std::chrono::milliseconds(impl_->config.connection_timeout_ms);

    bool connected = impl_->channel->WaitForConnected(deadline);

    if (connected) {
        VLOG(1) << "Connection successful";
    } else {
        LOG(WARNING) << "Connection to " << impl_->config.server_address
                     << " timed out after " << impl_->config.connection_timeout_ms << "ms";
    }

    return connected;
}

} // namespace ascnd

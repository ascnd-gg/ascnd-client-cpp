#pragma once

/**
 * @file client.hpp
 * @brief Ascnd API client for C++ games
 *
 * Thread-safe gRPC client for interacting with the Ascnd leaderboard API.
 * Designed for use in game engines like Unreal Engine and custom game projects.
 */

#include "types.hpp"
#include "ascnd.grpc.pb.h"

#include <string>
#include <memory>
#include <mutex>
#include <functional>
#include <chrono>
#include <future>
#include <stdexcept>

#include <grpcpp/grpcpp.h>

namespace ascnd {

// ============================================================================
// Logging API
// ============================================================================

/**
 * @brief Log level for the SDK
 *
 * Controls the verbosity of logging output.
 */
enum class LogLevel {
    kError = 0,    ///< Only errors
    kWarning = 1,  ///< Errors and warnings
    kInfo = 2,     ///< Normal operational messages
    kDebug = 3     ///< Detailed debug information (requires verbose=true)
};

/**
 * @brief Logging initialization options
 */
struct LoggingOptions {
    /// Minimum log level to output (default: kInfo)
    LogLevel min_level = LogLevel::kInfo;

    /// Program name for log prefix (default: "ascnd")
    std::string program_name = "ascnd";

    /// Enable colorized stderr output (default: true)
    bool colorize = true;
};

/**
 * @brief Initialize logging with custom options
 *
 * Call this function before creating any AscndClient instances to customize
 * logging behavior. If not called, logging will be auto-initialized with
 * sensible defaults when the first client is created.
 *
 * This function is thread-safe and can only be called once; subsequent calls
 * are ignored.
 *
 * @note Logging can only be initialized once per process. Calling
 *       ShutdownLogging() followed by InitLogging() will NOT re-initialize
 *       logging - the subsequent InitLogging() call is silently ignored.
 *       This is due to the use of std::call_once for thread-safety.
 *
 * @param options Logging configuration options
 *
 * Example:
 * @code
 * ascnd::LoggingOptions opts;
 * opts.min_level = ascnd::LogLevel::kDebug;
 * opts.program_name = "my_game";
 * ascnd::InitLogging(opts);
 *
 * // Now create clients
 * ascnd::AscndClient client(config);
 * @endcode
 */
void InitLogging(const LoggingOptions& options = LoggingOptions{});

/**
 * @brief Shutdown logging
 *
 * Call this before program exit to flush log buffers.
 * Safe to call multiple times.
 */
void ShutdownLogging();

// ============================================================================
// Client Configuration
// ============================================================================

/**
 * @brief Configuration options for AscndClient
 */
struct ClientConfig {
    /// gRPC server address (e.g., "api.ascnd.gg:443")
    std::string server_address;

    /// API key for authentication (sent as metadata)
    std::string api_key;

    /// Whether to use SSL/TLS (default: true)
    bool use_ssl = true;

    /// Connection timeout in milliseconds (default: 5000)
    int connection_timeout_ms = 5000;

    /// Request deadline in milliseconds (default: 10000)
    int request_timeout_ms = 10000;

    /// Number of retry attempts on transient failures (default: 3)
    int max_retries = 3;

    /// Base delay between retries in milliseconds (exponential backoff)
    int retry_delay_ms = 100;

    /// Custom User-Agent string (optional)
    std::string user_agent;

    /// Enable verbose logging (default: false)
    bool verbose = false;

    /**
     * @brief Validate the configuration
     * @throws std::invalid_argument if configuration is invalid
     */
    void validate() const {
        if (server_address.empty()) {
            throw std::invalid_argument("server_address cannot be empty");
        }
        if (connection_timeout_ms <= 0) {
            throw std::invalid_argument("connection_timeout_ms must be positive");
        }
        if (request_timeout_ms <= 0) {
            throw std::invalid_argument("request_timeout_ms must be positive");
        }
        if (max_retries < 0) {
            throw std::invalid_argument("max_retries cannot be negative");
        }
        if (retry_delay_ms < 0) {
            throw std::invalid_argument("retry_delay_ms cannot be negative");
        }
    }
};

/**
 * @brief Callback type for async operations
 */
template<typename T>
using AsyncCallback = std::function<void(Result<T>)>;

/**
 * @brief Thread-safe gRPC client for the Ascnd leaderboard API
 *
 * This client provides both synchronous and asynchronous methods
 * for interacting with the Ascnd API. All methods are thread-safe.
 *
 * Example usage:
 * @code
 * ascnd::ClientConfig config;
 * config.server_address = "api.ascnd.gg:443";
 * config.api_key = "your-api-key";
 *
 * ascnd::AscndClient client(config);
 *
 * // Submit a score
 * ascnd::SubmitScoreRequest req;
 * req.set_leaderboard_id("high-scores");
 * req.set_player_id("player123");
 * req.set_score(1000);
 *
 * auto result = client.submit_score(req);
 * if (result.is_ok()) {
 *     std::cout << "Rank: " << result.value().rank() << std::endl;
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
     * @brief Construct a client with server address and API key
     * @param server_address gRPC server address (e.g., "api.ascnd.gg:443")
     * @param api_key API key for authentication
     */
    AscndClient(const std::string& server_address, const std::string& api_key);

    /**
     * @brief Destructor - blocks until all pending async operations complete
     *
     * @note Callback-based async operations are tracked and the destructor
     *       will wait for them to finish before destroying the client.
     *       This prevents crashes from callbacks accessing destroyed objects.
     */
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
    // Asynchronous API (Future-based)
    // ========================================================================

    /**
     * @brief Submit a score asynchronously
     * @param request Score submission details
     * @return Future that will contain the result when complete
     *
     * @note The future can be waited on or checked for completion.
     *       Use .get() to retrieve the result (blocks if not ready).
     */
    [[nodiscard]] std::future<Result<SubmitScoreResponse>> submit_score_async(
        const SubmitScoreRequest& request
    );

    /**
     * @brief Get leaderboard entries asynchronously
     * @param request Leaderboard query parameters
     * @return Future that will contain the result when complete
     */
    [[nodiscard]] std::future<Result<GetLeaderboardResponse>> get_leaderboard_async(
        const GetLeaderboardRequest& request
    );

    /**
     * @brief Get a player's rank asynchronously
     * @param request Player rank query parameters
     * @return Future that will contain the result when complete
     */
    [[nodiscard]] std::future<Result<GetPlayerRankResponse>> get_player_rank_async(
        const GetPlayerRankRequest& request
    );

    // ========================================================================
    // Asynchronous API (Callback-based)
    // ========================================================================

    /**
     * @brief Submit a score asynchronously with callback
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
     * @brief Get leaderboard entries asynchronously with callback
     * @param request Leaderboard query parameters
     * @param callback Callback to invoke with the result
     */
    void get_leaderboard_async(
        const GetLeaderboardRequest& request,
        AsyncCallback<GetLeaderboardResponse> callback
    );

    /**
     * @brief Get a player's rank asynchronously with callback
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
     * @brief Get a copy of the current configuration
     * @return Copy of current client configuration (thread-safe)
     */
    [[nodiscard]] ClientConfig config() const;

    /**
     * @brief Test connectivity to the API
     * @return true if the gRPC channel is ready
     */
    [[nodiscard]] bool ping();

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ascnd

#pragma once

/**
 * @file types.hpp
 * @brief Request and response types for the Ascnd leaderboard API
 *
 * This header defines all the data structures used for communicating
 * with the Ascnd API. All types support JSON serialization via nlohmann/json.
 */

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace ascnd {

// Forward declarations for JSON support
using json = nlohmann::json;

/**
 * @brief Result type for API operations
 *
 * Since std::expected is C++23, we provide a simple Result type
 * that works with C++17 and is compatible with game engines.
 */
template<typename T>
class Result {
public:
    /// Create a successful result
    static Result ok(T value) {
        Result r;
        r.value_ = std::move(value);
        r.success_ = true;
        return r;
    }

    /// Create an error result
    static Result error(std::string error_message, int error_code = 0) {
        Result r;
        r.error_message_ = std::move(error_message);
        r.error_code_ = error_code;
        r.success_ = false;
        return r;
    }

    /// Check if the result is successful
    [[nodiscard]] bool is_ok() const noexcept { return success_; }

    /// Check if the result is an error
    [[nodiscard]] bool is_error() const noexcept { return !success_; }

    /// Implicit conversion to bool (true if successful)
    explicit operator bool() const noexcept { return success_; }

    /// Get the value (undefined behavior if is_error())
    [[nodiscard]] const T& value() const& { return *value_; }
    [[nodiscard]] T& value() & { return *value_; }
    [[nodiscard]] T&& value() && { return std::move(*value_); }

    /// Get the value or a default
    [[nodiscard]] T value_or(T default_value) const {
        return success_ ? *value_ : std::move(default_value);
    }

    /// Get the error message
    [[nodiscard]] const std::string& error() const { return error_message_; }

    /// Get the HTTP error code (0 if not an HTTP error)
    [[nodiscard]] int error_code() const noexcept { return error_code_; }

private:
    std::optional<T> value_;
    std::string error_message_;
    int error_code_ = 0;
    bool success_ = false;
};

// ============================================================================
// Request Types
// ============================================================================

/**
 * @brief Request to submit a score to a leaderboard
 */
struct SubmitScoreRequest {
    /// The leaderboard to submit the score to (required)
    std::string leaderboard_id;

    /// The player's unique identifier (required)
    std::string player_id;

    /// The score value (required)
    int64_t score = 0;

    /// Optional metadata (JSON-encoded game-specific data)
    std::optional<std::string> metadata;

    /// Optional idempotency key to prevent duplicate submissions
    std::optional<std::string> idempotency_key;
};

/**
 * @brief Request to get leaderboard entries
 */
struct GetLeaderboardRequest {
    /// The leaderboard to retrieve (required)
    std::string leaderboard_id;

    /// Maximum number of entries to return (default: 10, max: 100)
    std::optional<int32_t> limit;

    /// Number of entries to skip for pagination
    std::optional<int32_t> offset;

    /// Which period to retrieve: "current", "previous", or a timestamp
    std::optional<std::string> period;
};

/**
 * @brief Request to get a specific player's rank
 */
struct GetPlayerRankRequest {
    /// The leaderboard to query (required)
    std::string leaderboard_id;

    /// The player's unique identifier (required)
    std::string player_id;

    /// Which period to query: "current", "previous", or a timestamp
    std::optional<std::string> period;
};

// ============================================================================
// Response Types
// ============================================================================

/**
 * @brief Response from submitting a score
 */
struct SubmitScoreResponse {
    /// The unique identifier of the submitted score
    std::string score_id;

    /// The player's rank after this submission
    int32_t rank = 0;

    /// Whether this is the player's new best score for this period
    bool is_new_best = false;

    /// Whether the score was deduplicated (already submitted recently)
    bool was_deduplicated = false;
};

/**
 * @brief A single entry on the leaderboard
 */
struct LeaderboardEntry {
    /// The player's rank (1-indexed)
    int32_t rank = 0;

    /// The player's unique identifier
    std::string player_id;

    /// The player's score
    int64_t score = 0;

    /// When the score was submitted (ISO 8601 timestamp)
    std::string submitted_at;

    /// Optional metadata associated with the score
    std::optional<std::string> metadata;
};

/**
 * @brief Response containing leaderboard entries
 */
struct GetLeaderboardResponse {
    /// The leaderboard entries
    std::vector<LeaderboardEntry> entries;

    /// Approximate total number of entries
    int32_t total_entries = 0;

    /// Whether there are more entries after this page
    bool has_more = false;

    /// The start of the current period (ISO 8601 timestamp)
    std::string period_start;

    /// The end of the current period, if applicable (ISO 8601 timestamp)
    std::optional<std::string> period_end;
};

/**
 * @brief Response containing a player's rank information
 */
struct GetPlayerRankResponse {
    /// The player's rank (nullopt if not on leaderboard)
    std::optional<int32_t> rank;

    /// The player's current score (nullopt if not on leaderboard)
    std::optional<int64_t> score;

    /// The player's best score this period
    std::optional<int64_t> best_score;

    /// Total number of entries on this leaderboard
    int32_t total_entries = 0;

    /// The player's percentile (e.g., "top 5%")
    std::optional<std::string> percentile;
};

/**
 * @brief API error information
 */
struct ApiError {
    /// Error message
    std::string message;

    /// Error code (e.g., "INVALID_REQUEST", "NOT_FOUND")
    std::string code;

    /// HTTP status code
    int status = 0;
};

// ============================================================================
// JSON Serialization
// ============================================================================

// Helper macro for optional field serialization
#define ASCND_JSON_OPTIONAL(j, name, field) \
    if (field.has_value()) { j[name] = field.value(); }

#define ASCND_JSON_OPTIONAL_GET(j, name, field) \
    if (j.contains(name) && !j[name].is_null()) { field = j[name].get<decltype(field)::value_type>(); }

// SubmitScoreRequest
inline void to_json(json& j, const SubmitScoreRequest& r) {
    j = json{
        {"leaderboard_id", r.leaderboard_id},
        {"player_id", r.player_id},
        {"score", r.score}
    };
    ASCND_JSON_OPTIONAL(j, "metadata", r.metadata);
    ASCND_JSON_OPTIONAL(j, "idempotency_key", r.idempotency_key);
}

inline void from_json(const json& j, SubmitScoreRequest& r) {
    j.at("leaderboard_id").get_to(r.leaderboard_id);
    j.at("player_id").get_to(r.player_id);
    j.at("score").get_to(r.score);
    ASCND_JSON_OPTIONAL_GET(j, "metadata", r.metadata);
    ASCND_JSON_OPTIONAL_GET(j, "idempotency_key", r.idempotency_key);
}

// SubmitScoreResponse
inline void to_json(json& j, const SubmitScoreResponse& r) {
    j = json{
        {"score_id", r.score_id},
        {"rank", r.rank},
        {"is_new_best", r.is_new_best},
        {"was_deduplicated", r.was_deduplicated}
    };
}

inline void from_json(const json& j, SubmitScoreResponse& r) {
    j.at("score_id").get_to(r.score_id);
    j.at("rank").get_to(r.rank);
    j.at("is_new_best").get_to(r.is_new_best);
    j.at("was_deduplicated").get_to(r.was_deduplicated);
}

// GetLeaderboardRequest
inline void to_json(json& j, const GetLeaderboardRequest& r) {
    j = json{{"leaderboard_id", r.leaderboard_id}};
    ASCND_JSON_OPTIONAL(j, "limit", r.limit);
    ASCND_JSON_OPTIONAL(j, "offset", r.offset);
    ASCND_JSON_OPTIONAL(j, "period", r.period);
}

inline void from_json(const json& j, GetLeaderboardRequest& r) {
    j.at("leaderboard_id").get_to(r.leaderboard_id);
    ASCND_JSON_OPTIONAL_GET(j, "limit", r.limit);
    ASCND_JSON_OPTIONAL_GET(j, "offset", r.offset);
    ASCND_JSON_OPTIONAL_GET(j, "period", r.period);
}

// LeaderboardEntry
inline void to_json(json& j, const LeaderboardEntry& e) {
    j = json{
        {"rank", e.rank},
        {"player_id", e.player_id},
        {"score", e.score},
        {"submitted_at", e.submitted_at}
    };
    ASCND_JSON_OPTIONAL(j, "metadata", e.metadata);
}

inline void from_json(const json& j, LeaderboardEntry& e) {
    j.at("rank").get_to(e.rank);
    j.at("player_id").get_to(e.player_id);
    j.at("score").get_to(e.score);
    j.at("submitted_at").get_to(e.submitted_at);
    ASCND_JSON_OPTIONAL_GET(j, "metadata", e.metadata);
}

// GetLeaderboardResponse
inline void to_json(json& j, const GetLeaderboardResponse& r) {
    j = json{
        {"entries", r.entries},
        {"total_entries", r.total_entries},
        {"has_more", r.has_more},
        {"period_start", r.period_start}
    };
    ASCND_JSON_OPTIONAL(j, "period_end", r.period_end);
}

inline void from_json(const json& j, GetLeaderboardResponse& r) {
    j.at("entries").get_to(r.entries);
    j.at("total_entries").get_to(r.total_entries);
    j.at("has_more").get_to(r.has_more);
    j.at("period_start").get_to(r.period_start);
    ASCND_JSON_OPTIONAL_GET(j, "period_end", r.period_end);
}

// GetPlayerRankRequest
inline void to_json(json& j, const GetPlayerRankRequest& r) {
    j = json{
        {"leaderboard_id", r.leaderboard_id},
        {"player_id", r.player_id}
    };
    ASCND_JSON_OPTIONAL(j, "period", r.period);
}

inline void from_json(const json& j, GetPlayerRankRequest& r) {
    j.at("leaderboard_id").get_to(r.leaderboard_id);
    j.at("player_id").get_to(r.player_id);
    ASCND_JSON_OPTIONAL_GET(j, "period", r.period);
}

// GetPlayerRankResponse
inline void to_json(json& j, const GetPlayerRankResponse& r) {
    j = json{{"total_entries", r.total_entries}};
    ASCND_JSON_OPTIONAL(j, "rank", r.rank);
    ASCND_JSON_OPTIONAL(j, "score", r.score);
    ASCND_JSON_OPTIONAL(j, "best_score", r.best_score);
    ASCND_JSON_OPTIONAL(j, "percentile", r.percentile);
}

inline void from_json(const json& j, GetPlayerRankResponse& r) {
    j.at("total_entries").get_to(r.total_entries);
    ASCND_JSON_OPTIONAL_GET(j, "rank", r.rank);
    ASCND_JSON_OPTIONAL_GET(j, "score", r.score);
    ASCND_JSON_OPTIONAL_GET(j, "best_score", r.best_score);
    ASCND_JSON_OPTIONAL_GET(j, "percentile", r.percentile);
}

// ApiError
inline void to_json(json& j, const ApiError& e) {
    j = json{
        {"message", e.message},
        {"code", e.code},
        {"status", e.status}
    };
}

inline void from_json(const json& j, ApiError& e) {
    if (j.contains("message")) j.at("message").get_to(e.message);
    if (j.contains("code")) j.at("code").get_to(e.code);
    if (j.contains("status")) j.at("status").get_to(e.status);
}

#undef ASCND_JSON_OPTIONAL
#undef ASCND_JSON_OPTIONAL_GET

} // namespace ascnd

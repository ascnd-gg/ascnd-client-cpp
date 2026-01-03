#pragma once

/**
 * @file types.hpp
 * @brief Request and response types for the Ascnd leaderboard API
 *
 * This header provides type aliases for the generated protobuf types
 * and utility types for error handling.
 */

#include <string>
#include <optional>
#include <cstdint>

// Include generated protobuf types
#include "ascnd.pb.h"

namespace ascnd {

// ============================================================================
// Type Aliases for Proto Types
// ============================================================================

// Request types
using SubmitScoreRequest = ::ascnd::v1::SubmitScoreRequest;
using GetLeaderboardRequest = ::ascnd::v1::GetLeaderboardRequest;
using GetPlayerRankRequest = ::ascnd::v1::GetPlayerRankRequest;

// Response types
using SubmitScoreResponse = ::ascnd::v1::SubmitScoreResponse;
using GetLeaderboardResponse = ::ascnd::v1::GetLeaderboardResponse;
using GetPlayerRankResponse = ::ascnd::v1::GetPlayerRankResponse;

// Supporting types
using LeaderboardEntry = ::ascnd::v1::LeaderboardEntry;
using AnticheatResult = ::ascnd::v1::AnticheatResult;
using AnticheatViolation = ::ascnd::v1::AnticheatViolation;
using BracketInfo = ::ascnd::v1::BracketInfo;
using ViewInfo = ::ascnd::v1::ViewInfo;

// ============================================================================
// Result Type
// ============================================================================

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

    /// Get the gRPC error code (0 if not a gRPC error)
    [[nodiscard]] int error_code() const noexcept { return error_code_; }

private:
    std::optional<T> value_;
    std::string error_message_;
    int error_code_ = 0;
    bool success_ = false;
};

/**
 * @brief API error information
 */
struct ApiError {
    /// Error message
    std::string message;

    /// gRPC status code
    int status_code = 0;

    /// gRPC error details (if available)
    std::string details;
};

} // namespace ascnd

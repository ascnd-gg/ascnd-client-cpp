/**
 * @file result_test.cpp
 * @brief Unit tests for the Result<T> type
 */

#include <gtest/gtest.h>
#include "ascnd/types.hpp"
#include <string>
#include <utility>

namespace ascnd {
namespace {

// Test fixture for Result tests
class ResultTest : public ::testing::Test {
protected:
    // Helper struct for testing move semantics
    struct MoveTracker {
        int value;
        bool moved_from = false;

        explicit MoveTracker(int v) : value(v) {}
        MoveTracker(const MoveTracker& other) : value(other.value), moved_from(false) {}
        MoveTracker(MoveTracker&& other) noexcept : value(other.value), moved_from(false) {
            other.moved_from = true;
        }
        MoveTracker& operator=(const MoveTracker& other) {
            value = other.value;
            moved_from = false;
            return *this;
        }
        MoveTracker& operator=(MoveTracker&& other) noexcept {
            value = other.value;
            moved_from = false;
            other.moved_from = true;
            return *this;
        }
    };
};

// Test Result::ok() creates a successful result
TEST_F(ResultTest, OkCreatesSuccessResult) {
    auto result = Result<int>::ok(42);

    EXPECT_TRUE(result.is_ok());
    EXPECT_FALSE(result.is_error());
    EXPECT_TRUE(static_cast<bool>(result));
    EXPECT_EQ(result.value(), 42);
}

// Test Result::error() creates an error result
TEST_F(ResultTest, ErrorCreatesErrorResult) {
    auto result = Result<int>::error("Something went wrong", 5);

    EXPECT_FALSE(result.is_ok());
    EXPECT_TRUE(result.is_error());
    EXPECT_FALSE(static_cast<bool>(result));
    EXPECT_EQ(result.error(), "Something went wrong");
    EXPECT_EQ(result.error_code(), 5);
}

// Test Result::error() with default error code
TEST_F(ResultTest, ErrorWithDefaultCode) {
    auto result = Result<int>::error("Error message");

    EXPECT_TRUE(result.is_error());
    EXPECT_EQ(result.error(), "Error message");
    EXPECT_EQ(result.error_code(), 0);
}

// Test value() returns correct value for success
TEST_F(ResultTest, ValueReturnsCorrectValue) {
    auto result = Result<std::string>::ok("hello");

    EXPECT_EQ(result.value(), "hello");
}

// Test value_or() returns value for success
TEST_F(ResultTest, ValueOrReturnsValueOnSuccess) {
    auto result = Result<int>::ok(100);

    EXPECT_EQ(result.value_or(0), 100);
}

// Test value_or() returns default for error
TEST_F(ResultTest, ValueOrReturnsDefaultOnError) {
    auto result = Result<int>::error("failed");

    EXPECT_EQ(result.value_or(42), 42);
}

// Test value_or() with complex type
TEST_F(ResultTest, ValueOrWithComplexType) {
    auto result = Result<std::string>::error("failed");

    EXPECT_EQ(result.value_or("default"), "default");
}

// Test move semantics with ok()
TEST_F(ResultTest, MoveSemanticWithOk) {
    MoveTracker tracker(10);
    auto result = Result<MoveTracker>::ok(std::move(tracker));

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().value, 10);
    EXPECT_TRUE(tracker.moved_from);
}

// Test const value access
TEST_F(ResultTest, ConstValueAccess) {
    const auto result = Result<int>::ok(42);

    EXPECT_EQ(result.value(), 42);
}

// Test mutable value access
TEST_F(ResultTest, MutableValueAccess) {
    auto result = Result<int>::ok(42);
    result.value() = 100;

    EXPECT_EQ(result.value(), 100);
}

// Test rvalue value access
TEST_F(ResultTest, RvalueValueAccess) {
    auto result = Result<MoveTracker>::ok(MoveTracker(42));
    MoveTracker moved = std::move(result).value();

    EXPECT_EQ(moved.value, 42);
}

// Test with string type
TEST_F(ResultTest, WorksWithString) {
    auto ok_result = Result<std::string>::ok("success");
    auto error_result = Result<std::string>::error("failed", 1);

    EXPECT_TRUE(ok_result.is_ok());
    EXPECT_EQ(ok_result.value(), "success");

    EXPECT_TRUE(error_result.is_error());
    EXPECT_EQ(error_result.error(), "failed");
}

// Test with vector type
TEST_F(ResultTest, WorksWithVector) {
    std::vector<int> data = {1, 2, 3, 4, 5};
    auto result = Result<std::vector<int>>::ok(data);

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().size(), 5);
    EXPECT_EQ(result.value()[2], 3);
}

// Test error message is preserved correctly
TEST_F(ResultTest, ErrorMessagePreserved) {
    const std::string long_message = "This is a very long error message that should be preserved correctly";
    auto result = Result<int>::error(long_message, 42);

    EXPECT_EQ(result.error(), long_message);
    EXPECT_EQ(result.error_code(), 42);
}

// Test copy semantics
TEST_F(ResultTest, CopySemantics) {
    auto result1 = Result<int>::ok(42);
    auto result2 = result1;  // Copy

    EXPECT_TRUE(result1.is_ok());
    EXPECT_TRUE(result2.is_ok());
    EXPECT_EQ(result1.value(), 42);
    EXPECT_EQ(result2.value(), 42);
}

// Test move semantics for Result itself
TEST_F(ResultTest, MoveSemantics) {
    auto result1 = Result<std::string>::ok("hello");
    auto result2 = std::move(result1);

    EXPECT_TRUE(result2.is_ok());
    EXPECT_EQ(result2.value(), "hello");
}

// Test explicit bool conversion
TEST_F(ResultTest, ExplicitBoolConversion) {
    auto ok_result = Result<int>::ok(1);
    auto error_result = Result<int>::error("error");

    // These should work
    if (ok_result) {
        SUCCEED();
    } else {
        FAIL() << "ok_result should be true";
    }

    if (error_result) {
        FAIL() << "error_result should be false";
    } else {
        SUCCEED();
    }
}

}  // namespace
}  // namespace ascnd

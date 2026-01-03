/**
 * @file logging_test.cpp
 * @brief Unit tests for logging initialization
 */

#include <gtest/gtest.h>
#include "ascnd/client.hpp"

namespace ascnd {
namespace {

class LoggingTest : public ::testing::Test {
protected:
    void TearDown() override {
        // ShutdownLogging is safe to call multiple times
        ShutdownLogging();
    }
};

// Test that InitLogging with defaults doesn't crash
TEST_F(LoggingTest, InitLoggingWithDefaults) {
    EXPECT_NO_THROW({
        InitLogging();
    });
}

// Test that InitLogging with custom options works
TEST_F(LoggingTest, InitLoggingWithCustomOptions) {
    LoggingOptions opts;
    opts.min_level = LogLevel::kDebug;
    opts.program_name = "test_program";
    opts.colorize = false;

    EXPECT_NO_THROW({
        InitLogging(opts);
    });
}

// Test that auto-init occurs on client creation
TEST_F(LoggingTest, AutoInitOnClientCreation) {
    // Don't call InitLogging explicitly
    // Create a client - this should auto-init logging
    ClientConfig config;
    config.server_address = "localhost:50051";
    config.use_ssl = false;
    config.connection_timeout_ms = 50;
    config.request_timeout_ms = 50;
    config.max_retries = 0;

    EXPECT_NO_THROW({
        AscndClient client(config);
    });
}

// Test that verbose mode enables debug logging
TEST_F(LoggingTest, VerboseModeEnablesDebugLogging) {
    ClientConfig config;
    config.server_address = "localhost:50051";
    config.use_ssl = false;
    config.connection_timeout_ms = 50;
    config.request_timeout_ms = 50;
    config.max_retries = 0;
    config.verbose = true;

    EXPECT_NO_THROW({
        AscndClient client(config);
    });
}

// Test that ShutdownLogging is safe to call multiple times
TEST_F(LoggingTest, ShutdownLoggingIsSafe) {
    EXPECT_NO_THROW({
        ShutdownLogging();
        ShutdownLogging();  // Multiple calls should be safe
    });
}

// Test LogLevel enum values
TEST_F(LoggingTest, LogLevelValues) {
    EXPECT_EQ(static_cast<int>(LogLevel::kError), 0);
    EXPECT_EQ(static_cast<int>(LogLevel::kWarning), 1);
    EXPECT_EQ(static_cast<int>(LogLevel::kInfo), 2);
    EXPECT_EQ(static_cast<int>(LogLevel::kDebug), 3);
}

// Test LoggingOptions default values
TEST_F(LoggingTest, LoggingOptionsDefaults) {
    LoggingOptions opts;

    EXPECT_EQ(opts.min_level, LogLevel::kInfo);
    EXPECT_EQ(opts.program_name, "ascnd");
    EXPECT_TRUE(opts.colorize);
}

// Test that InitLogging can be called before any client creation
TEST_F(LoggingTest, ExplicitInitBeforeClient) {
    LoggingOptions opts;
    opts.min_level = LogLevel::kWarning;
    opts.program_name = "my_game";

    InitLogging(opts);

    ClientConfig config;
    config.server_address = "localhost:50051";
    config.use_ssl = false;
    config.connection_timeout_ms = 50;
    config.request_timeout_ms = 50;
    config.max_retries = 0;

    EXPECT_NO_THROW({
        AscndClient client(config);
    });
}

// Test all log levels can be set
TEST_F(LoggingTest, AllLogLevelsWork) {
    std::vector<LogLevel> levels = {
        LogLevel::kError,
        LogLevel::kWarning,
        LogLevel::kInfo,
        LogLevel::kDebug
    };

    for (auto level : levels) {
        LoggingOptions opts;
        opts.min_level = level;

        // Only the first call takes effect due to std::call_once
        // This test just verifies no crashes occur
        EXPECT_NO_THROW({
            InitLogging(opts);
        });
    }
}

}  // namespace
}  // namespace ascnd

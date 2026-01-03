/**
 * @file client_test.cpp
 * @brief Unit tests for AscndClient
 *
 * These tests verify the client's behavior without connecting to a real server.
 * Tests that require network calls are minimal and focus on error handling.
 */

#include <gtest/gtest.h>
#include "ascnd/client.hpp"
#include <chrono>
#include <thread>
#include <atomic>
#include <future>

namespace ascnd {
namespace {

class ClientTest : public ::testing::Test {
protected:
    ClientConfig valid_config;

    void SetUp() override {
        valid_config.server_address = "localhost:50051";
        valid_config.api_key = "test-key";
        valid_config.use_ssl = false;  // Disable SSL for local testing
        valid_config.connection_timeout_ms = 100;  // Short timeout for tests
        valid_config.request_timeout_ms = 100;
        valid_config.max_retries = 0;  // No retries for faster tests
    }
};

// Test client construction with valid config
TEST_F(ClientTest, ConstructWithValidConfig) {
    EXPECT_NO_THROW({
        AscndClient client(valid_config);
    });
}

// Test client construction with server address and api key
TEST_F(ClientTest, ConstructWithAddressAndKey) {
    EXPECT_NO_THROW({
        AscndClient client("localhost:50051", "api-key");
    });
}

// Test that client is movable
TEST_F(ClientTest, ClientIsMovable) {
    AscndClient client1(valid_config);
    AscndClient client2 = std::move(client1);

    // client2 should be usable
    auto config = client2.config();
    EXPECT_EQ(config.server_address, "localhost:50051");
}

// Test config() returns correct configuration
TEST_F(ClientTest, ConfigReturnsCorrectValues) {
    valid_config.api_key = "my-test-key";
    valid_config.max_retries = 5;
    valid_config.verbose = true;

    AscndClient client(valid_config);
    auto config = client.config();

    EXPECT_EQ(config.server_address, "localhost:50051");
    EXPECT_EQ(config.api_key, "my-test-key");
    EXPECT_EQ(config.max_retries, 5);
    EXPECT_TRUE(config.verbose);
}

// Test set_api_key updates the API key
TEST_F(ClientTest, SetApiKeyUpdatesKey) {
    AscndClient client(valid_config);

    client.set_api_key("new-api-key");
    auto config = client.config();

    EXPECT_EQ(config.api_key, "new-api-key");
}

// Test config() is thread-safe with set_api_key
TEST_F(ClientTest, ConfigThreadSafe) {
    AscndClient client(valid_config);
    std::atomic<bool> stop{false};
    std::atomic<int> reads{0};
    std::atomic<int> writes{0};

    // Reader thread
    auto reader = std::async(std::launch::async, [&]() {
        while (!stop) {
            auto config = client.config();
            (void)config.api_key;  // Access the key
            ++reads;
        }
    });

    // Writer thread
    auto writer = std::async(std::launch::async, [&]() {
        for (int i = 0; i < 100; ++i) {
            client.set_api_key("key-" + std::to_string(i));
            ++writes;
        }
    });

    writer.wait();
    stop = true;
    reader.wait();

    EXPECT_EQ(writes.load(), 100);
    EXPECT_GT(reads.load(), 0);
}

// Test that ping returns false for unreachable server
TEST_F(ClientTest, PingReturnsFalseForUnreachableServer) {
    valid_config.connection_timeout_ms = 50;  // Very short timeout
    AscndClient client(valid_config);

    bool connected = client.ping();
    EXPECT_FALSE(connected);
}

// Test submit_score fails gracefully for unreachable server
TEST_F(ClientTest, SubmitScoreFailsForUnreachableServer) {
    AscndClient client(valid_config);

    SubmitScoreRequest request;
    request.set_leaderboard_id("test-leaderboard");
    request.set_player_id("player1");
    request.set_score(100);

    auto result = client.submit_score(request);

    EXPECT_TRUE(result.is_error());
    EXPECT_NE(result.error_code(), 0);
}

// Test get_leaderboard fails gracefully for unreachable server
TEST_F(ClientTest, GetLeaderboardFailsForUnreachableServer) {
    AscndClient client(valid_config);

    GetLeaderboardRequest request;
    request.set_leaderboard_id("test-leaderboard");
    request.set_limit(10);

    auto result = client.get_leaderboard(request);

    EXPECT_TRUE(result.is_error());
}

// Test get_player_rank fails gracefully for unreachable server
TEST_F(ClientTest, GetPlayerRankFailsForUnreachableServer) {
    AscndClient client(valid_config);

    GetPlayerRankRequest request;
    request.set_leaderboard_id("test-leaderboard");
    request.set_player_id("player1");

    auto result = client.get_player_rank(request);

    EXPECT_TRUE(result.is_error());
}

// Test convenience methods compile and work
TEST_F(ClientTest, ConvenienceMethodsWork) {
    AscndClient client(valid_config);

    // These will fail due to no server, but should not throw
    auto submit_result = client.submit_score("leaderboard", "player", 100);
    EXPECT_TRUE(submit_result.is_error());

    auto leaderboard_result = client.get_leaderboard("leaderboard", 10);
    EXPECT_TRUE(leaderboard_result.is_error());

    auto rank_result = client.get_player_rank("leaderboard", "player");
    EXPECT_TRUE(rank_result.is_error());
}

// Test async future-based API returns valid future
TEST_F(ClientTest, AsyncFutureApiReturnsValidFuture) {
    AscndClient client(valid_config);

    SubmitScoreRequest request;
    request.set_leaderboard_id("test-leaderboard");
    request.set_player_id("player1");
    request.set_score(100);

    auto future = client.submit_score_async(request);

    EXPECT_TRUE(future.valid());

    // Wait for result
    auto result = future.get();
    EXPECT_TRUE(result.is_error());  // No server running
}

// Test async callback-based API invokes callback
TEST_F(ClientTest, AsyncCallbackApiInvokesCallback) {
    AscndClient client(valid_config);

    SubmitScoreRequest request;
    request.set_leaderboard_id("test-leaderboard");
    request.set_player_id("player1");
    request.set_score(100);

    std::promise<Result<SubmitScoreResponse>> promise;
    auto future = promise.get_future();

    client.submit_score_async(request, [&promise](Result<SubmitScoreResponse> result) {
        promise.set_value(std::move(result));
    });

    // Wait for callback with timeout
    auto status = future.wait_for(std::chrono::seconds(5));
    EXPECT_EQ(status, std::future_status::ready);

    auto result = future.get();
    EXPECT_TRUE(result.is_error());  // No server running
}

// Test that client destructor waits for pending async operations
TEST_F(ClientTest, DestructorWaitsForPendingOperations) {
    std::atomic<bool> callback_called{false};

    {
        AscndClient client(valid_config);

        SubmitScoreRequest request;
        request.set_leaderboard_id("test");
        request.set_player_id("player");
        request.set_score(1);

        client.submit_score_async(request, [&callback_called](Result<SubmitScoreResponse>) {
            callback_called = true;
        });

        // Client goes out of scope here - should wait for async op
    }

    // Callback should have been invoked before destructor returned
    EXPECT_TRUE(callback_called);
}

// Test multiple async operations complete before destructor
TEST_F(ClientTest, MultipleAsyncOperationsComplete) {
    std::atomic<int> callbacks_completed{0};

    {
        AscndClient client(valid_config);

        for (int i = 0; i < 5; ++i) {
            SubmitScoreRequest request;
            request.set_leaderboard_id("test");
            request.set_player_id("player");
            request.set_score(i);

            client.submit_score_async(request, [&callbacks_completed](Result<SubmitScoreResponse>) {
                ++callbacks_completed;
            });
        }
    }

    EXPECT_EQ(callbacks_completed.load(), 5);
}

// Test async leaderboard query
TEST_F(ClientTest, AsyncLeaderboardQuery) {
    AscndClient client(valid_config);

    GetLeaderboardRequest request;
    request.set_leaderboard_id("test-leaderboard");

    auto future = client.get_leaderboard_async(request);
    auto result = future.get();

    EXPECT_TRUE(result.is_error());  // No server
}

// Test async player rank query
TEST_F(ClientTest, AsyncPlayerRankQuery) {
    AscndClient client(valid_config);

    GetPlayerRankRequest request;
    request.set_leaderboard_id("test-leaderboard");
    request.set_player_id("player1");

    auto future = client.get_player_rank_async(request);
    auto result = future.get();

    EXPECT_TRUE(result.is_error());  // No server
}

// Test that callback exceptions don't crash the destructor
TEST_F(ClientTest, CallbackExceptionDoesNotCrash) {
    // This test verifies that if a user callback throws an exception,
    // the client destructor doesn't crash (which would happen if the
    // exception propagated through std::future::wait())
    {
        AscndClient client(valid_config);

        SubmitScoreRequest request;
        request.set_leaderboard_id("test");
        request.set_player_id("player");
        request.set_score(100);

        client.submit_score_async(request, [](Result<SubmitScoreResponse>) {
            throw std::runtime_error("Test exception from callback");
        });
    }
    // If we reach here without crashing, the test passed
    SUCCEED();
}

// Test that multiple callbacks with exceptions don't crash
TEST_F(ClientTest, MultipleCallbackExceptionsDoNotCrash) {
    {
        AscndClient client(valid_config);

        for (int i = 0; i < 3; ++i) {
            SubmitScoreRequest request;
            request.set_leaderboard_id("test");
            request.set_player_id("player");
            request.set_score(i);

            client.submit_score_async(request, [](Result<SubmitScoreResponse>) {
                throw std::runtime_error("Exception in callback");
            });
        }
    }
    SUCCEED();
}

}  // namespace
}  // namespace ascnd

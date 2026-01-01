/**
 * @file basic_usage.cpp
 * @brief Example demonstrating basic usage of the Ascnd C++ client
 *
 * This example shows how to:
 * - Initialize the client
 * - Submit scores
 * - Retrieve leaderboards
 * - Get player rankings
 * - Use async operations
 */

#include <ascnd/client.hpp>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>

// Helper function to print a separator line
void print_separator() {
    std::cout << std::string(60, '-') << std::endl;
}

// Example 1: Basic synchronous operations
void example_sync_operations(ascnd::AscndClient& client) {
    std::cout << "\n=== Example 1: Synchronous Operations ===\n" << std::endl;

    // Submit a score
    std::cout << "Submitting score..." << std::endl;

    ascnd::SubmitScoreRequest submit_req;
    submit_req.leaderboard_id = "high-scores";
    submit_req.player_id = "player_001";
    submit_req.score = 15000;
    submit_req.metadata = R"({"level": 5, "character": "warrior"})";

    auto submit_result = client.submit_score(submit_req);

    if (submit_result.is_ok()) {
        auto& response = submit_result.value();
        std::cout << "Score submitted successfully!" << std::endl;
        std::cout << "  Score ID: " << response.score_id << std::endl;
        std::cout << "  Rank: " << response.rank << std::endl;
        std::cout << "  New best: " << (response.is_new_best ? "Yes" : "No") << std::endl;
    } else {
        std::cerr << "Failed to submit score: " << submit_result.error() << std::endl;
        std::cerr << "  Error code: " << submit_result.error_code() << std::endl;
    }

    print_separator();

    // Get leaderboard
    std::cout << "Fetching leaderboard..." << std::endl;

    ascnd::GetLeaderboardRequest leaderboard_req;
    leaderboard_req.leaderboard_id = "high-scores";
    leaderboard_req.limit = 10;

    auto leaderboard_result = client.get_leaderboard(leaderboard_req);

    if (leaderboard_result.is_ok()) {
        auto& response = leaderboard_result.value();
        std::cout << "Leaderboard retrieved! (" << response.total_entries << " total entries)" << std::endl;
        std::cout << "Period: " << response.period_start;
        if (response.period_end.has_value()) {
            std::cout << " to " << response.period_end.value();
        }
        std::cout << std::endl << std::endl;

        std::cout << std::setw(6) << "Rank"
                  << std::setw(20) << "Player"
                  << std::setw(12) << "Score"
                  << std::endl;
        std::cout << std::string(38, '-') << std::endl;

        for (const auto& entry : response.entries) {
            std::cout << std::setw(6) << entry.rank
                      << std::setw(20) << entry.player_id
                      << std::setw(12) << entry.score
                      << std::endl;
        }

        if (response.has_more) {
            std::cout << "... and more entries available" << std::endl;
        }
    } else {
        std::cerr << "Failed to get leaderboard: " << leaderboard_result.error() << std::endl;
    }

    print_separator();

    // Get player rank
    std::cout << "Getting player rank..." << std::endl;

    auto rank_result = client.get_player_rank("high-scores", "player_001");

    if (rank_result.is_ok()) {
        auto& response = rank_result.value();
        if (response.rank.has_value()) {
            std::cout << "Player ranking:" << std::endl;
            std::cout << "  Rank: " << response.rank.value() << std::endl;
            if (response.score.has_value()) {
                std::cout << "  Score: " << response.score.value() << std::endl;
            }
            if (response.best_score.has_value()) {
                std::cout << "  Best score: " << response.best_score.value() << std::endl;
            }
            if (response.percentile.has_value()) {
                std::cout << "  Percentile: " << response.percentile.value() << std::endl;
            }
            std::cout << "  Total players: " << response.total_entries << std::endl;
        } else {
            std::cout << "Player not found on this leaderboard." << std::endl;
        }
    } else {
        std::cerr << "Failed to get player rank: " << rank_result.error() << std::endl;
    }
}

// Example 2: Using convenience methods
void example_convenience_methods(ascnd::AscndClient& client) {
    std::cout << "\n=== Example 2: Convenience Methods ===\n" << std::endl;

    // Quick score submission
    auto result = client.submit_score("daily-challenge", "player_002", 8500);
    if (result.is_ok()) {
        std::cout << "Quick submit - Rank: " << result.value().rank << std::endl;
    }

    // Quick leaderboard fetch (top 5)
    auto leaderboard = client.get_leaderboard("daily-challenge", 5);
    if (leaderboard.is_ok()) {
        std::cout << "Top 5 on daily-challenge:" << std::endl;
        for (const auto& entry : leaderboard.value().entries) {
            std::cout << "  #" << entry.rank << " " << entry.player_id
                      << " - " << entry.score << std::endl;
        }
    }

    // Quick rank check
    auto rank = client.get_player_rank("daily-challenge", "player_002");
    if (rank.is_ok() && rank.value().rank.has_value()) {
        std::cout << "Player 002 is ranked #" << rank.value().rank.value() << std::endl;
    }
}

// Example 3: Asynchronous operations
void example_async_operations(ascnd::AscndClient& client) {
    std::cout << "\n=== Example 3: Asynchronous Operations ===\n" << std::endl;

    std::atomic<int> pending_ops{3};

    // Async score submission
    ascnd::SubmitScoreRequest submit_req;
    submit_req.leaderboard_id = "async-test";
    submit_req.player_id = "async_player";
    submit_req.score = 12345;

    client.submit_score_async(submit_req, [&pending_ops](ascnd::Result<ascnd::SubmitScoreResponse> result) {
        if (result.is_ok()) {
            std::cout << "[Async] Score submitted, rank: " << result.value().rank << std::endl;
        } else {
            std::cerr << "[Async] Submit failed: " << result.error() << std::endl;
        }
        --pending_ops;
    });

    // Async leaderboard fetch
    ascnd::GetLeaderboardRequest leaderboard_req;
    leaderboard_req.leaderboard_id = "async-test";
    leaderboard_req.limit = 5;

    client.get_leaderboard_async(leaderboard_req, [&pending_ops](ascnd::Result<ascnd::GetLeaderboardResponse> result) {
        if (result.is_ok()) {
            std::cout << "[Async] Leaderboard fetched, "
                      << result.value().entries.size() << " entries" << std::endl;
        } else {
            std::cerr << "[Async] Leaderboard failed: " << result.error() << std::endl;
        }
        --pending_ops;
    });

    // Async rank check
    ascnd::GetPlayerRankRequest rank_req;
    rank_req.leaderboard_id = "async-test";
    rank_req.player_id = "async_player";

    client.get_player_rank_async(rank_req, [&pending_ops](ascnd::Result<ascnd::GetPlayerRankResponse> result) {
        if (result.is_ok() && result.value().rank.has_value()) {
            std::cout << "[Async] Player rank: " << result.value().rank.value() << std::endl;
        } else if (result.is_error()) {
            std::cerr << "[Async] Rank check failed: " << result.error() << std::endl;
        }
        --pending_ops;
    });

    std::cout << "Waiting for async operations to complete..." << std::endl;

    // Wait for all async operations
    while (pending_ops > 0) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "All async operations completed!" << std::endl;
}

// Example 4: Error handling
void example_error_handling(ascnd::AscndClient& client) {
    std::cout << "\n=== Example 4: Error Handling ===\n" << std::endl;

    // Try to access a non-existent leaderboard
    auto result = client.get_leaderboard("non-existent-leaderboard");

    if (result.is_error()) {
        std::cout << "Expected error occurred:" << std::endl;
        std::cout << "  Message: " << result.error() << std::endl;
        std::cout << "  HTTP Code: " << result.error_code() << std::endl;

        // Check specific error codes
        if (result.error_code() == 404) {
            std::cout << "  -> Leaderboard not found" << std::endl;
        } else if (result.error_code() == 401) {
            std::cout << "  -> Authentication failed" << std::endl;
        } else if (result.error_code() == 429) {
            std::cout << "  -> Rate limited, try again later" << std::endl;
        }
    }

    // Using value_or for defaults
    auto safe_result = client.get_leaderboard("maybe-exists");
    ascnd::GetLeaderboardResponse default_response;
    default_response.total_entries = 0;
    default_response.has_more = false;

    auto response = safe_result.is_ok() ?
        safe_result.value() : default_response;
    std::cout << "Total entries (with fallback): " << response.total_entries << std::endl;
}

// Example 5: Configuration
void example_configuration() {
    std::cout << "\n=== Example 5: Client Configuration ===\n" << std::endl;

    // Full configuration
    ascnd::ClientConfig config;
    config.base_url = "https://api.ascnd.gg";
    config.api_key = "your-api-key-here";
    config.connection_timeout_ms = 3000;
    config.read_timeout_ms = 5000;
    config.write_timeout_ms = 5000;
    config.max_retries = 3;
    config.retry_delay_ms = 100;
    config.user_agent = "MyGame/1.0.0";
    config.verbose = true;

    ascnd::AscndClient configured_client(config);

    std::cout << "Client configured with:" << std::endl;
    std::cout << "  Base URL: " << configured_client.config().base_url << std::endl;
    std::cout << "  Timeout: " << configured_client.config().connection_timeout_ms << "ms" << std::endl;
    std::cout << "  Max retries: " << configured_client.config().max_retries << std::endl;

    // Update API key at runtime
    configured_client.set_api_key("new-api-key");
    std::cout << "API key updated at runtime" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "Ascnd C++ Client - Basic Usage Examples" << std::endl;
    std::cout << "========================================" << std::endl;

    // Get API configuration from environment or command line
    std::string base_url = "https://api.ascnd.gg";
    std::string api_key = "demo-api-key";

    if (argc >= 3) {
        base_url = argv[1];
        api_key = argv[2];
    } else if (const char* env_url = std::getenv("ASCND_API_URL")) {
        base_url = env_url;
    }

    if (const char* env_key = std::getenv("ASCND_API_KEY")) {
        api_key = env_key;
    }

    std::cout << "Using API: " << base_url << std::endl;

    // Create client
    ascnd::AscndClient client(base_url, api_key);

    // Test connectivity
    std::cout << "Testing connection... ";
    if (client.ping()) {
        std::cout << "OK" << std::endl;
    } else {
        std::cout << "Failed (continuing anyway for demo)" << std::endl;
    }

    // Run examples
    try {
        example_sync_operations(client);
        example_convenience_methods(client);
        example_async_operations(client);
        example_error_handling(client);
        example_configuration();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "\n=== All examples completed ===" << std::endl;
    return 0;
}

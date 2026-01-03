#include <ascnd/client.hpp>
#include <iostream>
#include <cstdlib>

int main() {
    const char* api_key = std::getenv("ASCND_API_KEY");
    const char* leaderboard_id = std::getenv("LEADERBOARD_ID");

    if (!api_key || !leaderboard_id) {
        std::cerr << "Error: Set ASCND_API_KEY and LEADERBOARD_ID environment variables\n";
        return 1;
    }

    // Use server address with port for gRPC
    ascnd::AscndClient client("api.ascnd.gg:443", api_key);

    // Submit score with metadata
    ascnd::SubmitScoreRequest submit_req;
    submit_req.set_leaderboard_id(leaderboard_id);
    submit_req.set_player_id("player_meta_001");
    submit_req.set_score(75000);
    submit_req.set_metadata(R"({"character":"warrior","level":15,"powerups":["speed","shield"]})");

    auto submit_result = client.submit_score(submit_req);

    if (submit_result.is_ok()) {
        std::cout << "Score submitted with metadata! Rank: #" << submit_result.value().rank() << "\n\n";
    } else {
        std::cerr << "Submit error: " << submit_result.error() << "\n";
        return 1;
    }

    // Fetch current period leaderboard
    ascnd::GetLeaderboardRequest lb_req;
    lb_req.set_leaderboard_id(leaderboard_id);
    lb_req.set_limit(5);
    lb_req.set_period("current");

    auto lb_result = client.get_leaderboard(lb_req);

    if (lb_result.is_ok()) {
        const auto& lb = lb_result.value();

        std::cout << "Current Period: " << lb.period_start() << "\n";
        if (lb.has_period_end()) {
            std::cout << "Ends: " << lb.period_end() << "\n";
        }
        std::cout << "\nTop 5 with metadata:\n\n";

        for (const auto& entry : lb.entries()) {
            std::cout << "#" << entry.rank() << " " << entry.player_id() << ": " << entry.score() << "\n";
            if (entry.has_metadata()) {
                std::cout << "   Metadata: " << entry.metadata() << "\n";
            }
        }
    } else {
        std::cerr << "Leaderboard error: " << lb_result.error() << "\n";
        return 1;
    }

    return 0;
}

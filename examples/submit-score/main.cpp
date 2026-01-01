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
    
    ascnd::AscndClient client("https://api.ascnd.gg", api_key);
    
    auto result = client.submit_score(leaderboard_id, "player_example_001", 42500);
    
    if (result.is_ok()) {
        std::cout << "Score submitted!\n";
        std::cout << "  Rank: #" << result.value().rank << "\n";
        std::cout << "  New personal best: " << (result.value().is_new_best ? "Yes!" : "No") << "\n";
    } else {
        std::cerr << "Error: " << result.error() << "\n";
        return 1;
    }
    
    return 0;
}

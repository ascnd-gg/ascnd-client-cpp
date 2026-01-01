#include <ascnd/client.hpp>
#include <iostream>
#include <iomanip>
#include <cstdlib>

int main() {
    const char* api_key = std::getenv("ASCND_API_KEY");
    const char* leaderboard_id = std::getenv("LEADERBOARD_ID");
    
    if (!api_key || !leaderboard_id) {
        std::cerr << "Error: Set ASCND_API_KEY and LEADERBOARD_ID environment variables\n";
        return 1;
    }
    
    ascnd::AscndClient client("https://api.ascnd.gg", api_key);
    
    auto result = client.get_leaderboard(leaderboard_id, 10);
    
    if (result.is_ok()) {
        auto& lb = result.value();
        
        std::cout << "Top 10 Leaderboard (" << lb.total_entries << " total players)\n\n";
        std::cout << "Rank  | Player             | Score\n";
        std::cout << "------+--------------------+------------\n";
        
        for (const auto& entry : lb.entries) {
            std::string player = entry.player_id;
            if (player.length() > 18) player = player.substr(0, 18);
            
            std::cout << std::setw(4) << entry.rank << "  | "
                      << std::left << std::setw(18) << player << " | "
                      << std::right << std::setw(10) << entry.score << "\n";
        }
        
        if (lb.has_more) {
            std::cout << "\n... and more entries\n";
        }
    } else {
        std::cerr << "Error: " << result.error() << "\n";
        return 1;
    }
    
    return 0;
}

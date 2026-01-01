# Ascnd C++ Client

A modern C++17 client library for the [Ascnd](https://ascnd.gg) leaderboard API. Designed for game engines like Unreal Engine and custom C++ game projects.

## Features

- **C++17 compatible** - Works with most game engines and build systems
- **Header-only option** - Easy integration, no separate compilation needed
- **Thread-safe** - All operations are thread-safe by default
- **Async support** - Non-blocking operations with callbacks
- **Minimal dependencies** - Only nlohmann/json and cpp-httplib (both header-only)
- **Automatic retries** - Built-in retry logic with exponential backoff
- **Error handling** - Result type for safe error handling (no exceptions required)

## Quick Start

```cpp
#include <ascnd/client.hpp>
#include <iostream>

int main() {
    // Create client
    ascnd::AscndClient client("https://api.ascnd.gg", "your-api-key");

    // Submit a score
    auto result = client.submit_score("high-scores", "player123", 15000);

    if (result.is_ok()) {
        std::cout << "Rank: " << result.value().rank << std::endl;
    } else {
        std::cerr << "Error: " << result.error() << std::endl;
    }

    return 0;
}
```

## Examples

Stand-alone example projects are available in the [`examples/`](./examples) directory:

| Example | Description |
|---------|-------------|
| [`submit-score`](./examples/submit-score) | Submit a score and display the resulting rank |
| [`leaderboard`](./examples/leaderboard) | Fetch and display the top 10 leaderboard entries |
| [`metadata-periods`](./examples/metadata-periods) | Submit scores with metadata and query by time period |

Each example can be built standalone or as part of the SDK:

```bash
# Standalone build
cd examples/submit-score
mkdir build && cd build
cmake ..
cmake --build .
export ASCND_API_KEY=your_api_key
export LEADERBOARD_ID=your_leaderboard_id
./submit-score

# Or build all examples with the SDK
mkdir build && cd build
cmake .. -DASCND_BUILD_EXAMPLES=ON
cmake --build .
```

## Installation

### Option 1: CMake FetchContent (Recommended)

Add to your `CMakeLists.txt`:

```cmake
include(FetchContent)
FetchContent_Declare(
    ascnd-client
    GIT_REPOSITORY https://github.com/ascnd-gg/ascnd-client-cpp.git
    GIT_TAG v1.0.0
)
FetchContent_MakeAvailable(ascnd-client)

target_link_libraries(your_target PRIVATE ascnd-client)
```

### Option 2: vcpkg

```bash
# Add to vcpkg.json
{
    "dependencies": ["ascnd-client"]
}

# Or install directly
vcpkg install ascnd-client
```

### Option 3: Manual Integration

1. Copy the `include/ascnd` directory to your project
2. Add the include path to your build system
3. Ensure nlohmann/json and cpp-httplib are available
4. For header-only mode, define `ASCND_HEADER_ONLY` before including

```cpp
#define ASCND_HEADER_ONLY
#include <ascnd/client.hpp>
```

### Option 4: Build as Static Library

```bash
mkdir build && cd build
cmake .. -DASCND_HEADER_ONLY=OFF
cmake --build .
cmake --install . --prefix /usr/local
```

## Build Requirements

- CMake 3.16+
- C++17 compatible compiler
- OpenSSL (optional, for HTTPS support)

### Dependencies (automatically fetched)

- [nlohmann/json](https://github.com/nlohmann/json) v3.11+ (header-only)
- [cpp-httplib](https://github.com/yhirose/cpp-httplib) v0.15+ (header-only)

## Usage Guide

### Client Configuration

```cpp
ascnd::ClientConfig config;
config.base_url = "https://api.ascnd.gg";
config.api_key = "your-api-key";
config.connection_timeout_ms = 5000;
config.read_timeout_ms = 10000;
config.max_retries = 3;
config.user_agent = "MyGame/1.0.0";

ascnd::AscndClient client(config);
```

### Submitting Scores

```cpp
// Simple submission
auto result = client.submit_score("leaderboard-id", "player-id", 1000);

// With metadata and idempotency key
ascnd::SubmitScoreRequest request;
request.leaderboard_id = "high-scores";
request.player_id = "player123";
request.score = 15000;
request.metadata = R"({"level": 5, "character": "warrior"})";
request.idempotency_key = "unique-key-123";

auto result = client.submit_score(request);

if (result.is_ok()) {
    auto& response = result.value();
    std::cout << "Score ID: " << response.score_id << std::endl;
    std::cout << "Rank: " << response.rank << std::endl;
    std::cout << "New best: " << response.is_new_best << std::endl;
}
```

### Getting Leaderboards

```cpp
// Simple query
auto result = client.get_leaderboard("high-scores", 10);

// With pagination
ascnd::GetLeaderboardRequest request;
request.leaderboard_id = "high-scores";
request.limit = 25;
request.offset = 50;
request.period = "current";

auto result = client.get_leaderboard(request);

if (result.is_ok()) {
    for (const auto& entry : result.value().entries) {
        std::cout << "#" << entry.rank << " "
                  << entry.player_id << " - "
                  << entry.score << std::endl;
    }
}
```

### Getting Player Rank

```cpp
auto result = client.get_player_rank("high-scores", "player123");

if (result.is_ok()) {
    auto& response = result.value();
    if (response.rank.has_value()) {
        std::cout << "Rank: " << response.rank.value() << std::endl;
        std::cout << "Percentile: " << response.percentile.value_or("N/A") << std::endl;
    } else {
        std::cout << "Player not on leaderboard" << std::endl;
    }
}
```

### Async Operations

```cpp
// Non-blocking score submission
client.submit_score_async(request, [](ascnd::Result<ascnd::SubmitScoreResponse> result) {
    if (result.is_ok()) {
        // Update UI on main thread
        std::cout << "Rank: " << result.value().rank << std::endl;
    }
});

// Non-blocking leaderboard fetch
client.get_leaderboard_async(request, [](ascnd::Result<ascnd::GetLeaderboardResponse> result) {
    if (result.is_ok()) {
        // Process leaderboard data
    }
});
```

### Error Handling

```cpp
auto result = client.submit_score("leaderboard", "player", 1000);

if (result.is_error()) {
    std::cerr << "Error: " << result.error() << std::endl;

    switch (result.error_code()) {
        case 401:
            // Invalid API key
            break;
        case 404:
            // Leaderboard not found
            break;
        case 429:
            // Rate limited
            break;
        default:
            // Other error
            break;
    }
}

// Or use value_or for defaults
auto score = result.value_or(default_response);
```

## Links

- [Documentation](https://ascnd.gg/docs/sdks/cpp)
- [GitHub](https://github.com/ascnd-gg/ascnd-client-cpp)
- [Examples](https://github.com/ascnd-gg/ascnd-client-cpp/tree/main/examples)

## License

MIT License - See [LICENSE](LICENSE) for details.

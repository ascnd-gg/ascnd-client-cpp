# Ascnd C++ Client

> [!WARNING]
> This project is under active development. Expect bugs. Please report issues via the [issue tracker](../../issues).

A modern C++17 client library for the [Ascnd](https://ascnd.gg) leaderboard API. Designed for game engines like Unreal Engine and custom C++ game projects.

## Features

- **C++17 compatible** - Works with most game engines and build systems
- **gRPC + Protobuf** - Type-safe API with generated protobuf types
- **Thread-safe** - All operations are thread-safe by default
- **Async support** - Non-blocking operations with callbacks
- **Automatic retries** - Built-in retry logic with exponential backoff
- **Error handling** - Result type for safe error handling (no exceptions required)
- **Anticheat** - Server-side anticheat validation with violation reporting
- **Brackets** - Player skill bracket assignment with colors
- **Metadata Views** - Query scores with custom metadata projections
- **Global Rank** - Track player ranking across all time

## Quick Start

```cpp
#include <ascnd/client.hpp>
#include <iostream>

int main() {
    // Create client
    ascnd::AscndClient client("api.ascnd.gg:443", "your-api-key");

    // Build a submit score request using protobuf setters
    ascnd::SubmitScoreRequest req;
    req.set_leaderboard_id("high-scores");
    req.set_player_id("player123");
    req.set_score(15000);

    // Submit the score
    auto result = client.submit_score(req);

    if (result.is_ok()) {
        const auto& response = result.value();
        std::cout << "Rank: " << response.rank() << std::endl;
        std::cout << "New best: " << response.is_new_best() << std::endl;
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

# Fetch gRPC and Protobuf
FetchContent_Declare(
    grpc
    GIT_REPOSITORY https://github.com/grpc/grpc.git
    GIT_TAG v1.60.0
)
set(gRPC_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(gRPC_BUILD_GRPC_PYTHON_PLUGIN OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(grpc)

# Fetch Ascnd client
FetchContent_Declare(
    ascnd-client
    GIT_REPOSITORY https://github.com/ascnd-gg/ascnd-client-cpp.git
    GIT_TAG v2.0.0
)
FetchContent_MakeAvailable(ascnd-client)

target_link_libraries(your_target PRIVATE ascnd-client)
```

### Option 2: vcpkg

```bash
# Install dependencies
vcpkg install grpc protobuf

# Add to vcpkg.json
{
    "dependencies": ["ascnd-client", "grpc", "protobuf"]
}

# Or install directly
vcpkg install ascnd-client
```

### Option 3: Manual Integration

1. Install gRPC and Protobuf on your system
2. Copy the `include/ascnd` directory to your project
3. Generate protobuf types from `.proto` files or use pre-generated headers
4. Add the include path to your build system

```cpp
#include <ascnd/client.hpp>
```

### Option 4: Build as Static Library

```bash
mkdir build && cd build
cmake .. -DASCND_BUILD_STATIC=ON
cmake --build .
cmake --install . --prefix /usr/local
```

## Build Requirements

- CMake 3.16+
- C++17 compatible compiler
- gRPC 1.50+
- Protobuf 3.20+

### Dependencies

- [gRPC](https://github.com/grpc/grpc) v1.50+
- [Protobuf](https://github.com/protocolbuffers/protobuf) v3.20+

## Usage Guide

### Client Configuration

```cpp
ascnd::ClientConfig config;
config.server_address = "api.ascnd.gg:443";
config.api_key = "your-api-key";
config.use_ssl = true;
config.request_timeout_ms = 10000;

ascnd::AscndClient client(config);
```

### Submitting Scores

```cpp
// Build request with protobuf setters
ascnd::SubmitScoreRequest req;
req.set_leaderboard_id("high-scores");
req.set_player_id("player123");
req.set_score(15000);
req.set_metadata(R"({"level": 5, "character": "warrior"})");
req.set_idempotency_key("unique-key-123");

auto result = client.submit_score(req);

if (result.is_ok()) {
    const auto& response = result.value();
    std::cout << "Score ID: " << response.score_id() << std::endl;
    std::cout << "Rank: " << response.rank() << std::endl;
    std::cout << "New best: " << response.is_new_best() << std::endl;

    // Check anticheat result (if enabled)
    if (response.has_anticheat()) {
        if (response.anticheat().passed()) {
            std::cout << "Anticheat: passed" << std::endl;
        } else {
            std::cout << "Anticheat action: " << response.anticheat().action() << std::endl;
        }
    }
}
```

### Getting Leaderboards

```cpp
// Build request with pagination and optional view filtering
ascnd::GetLeaderboardRequest req;
req.set_leaderboard_id("high-scores");
req.set_limit(25);
req.set_offset(0);
req.set_period("current");
req.set_view_slug("na-region");  // Optional: filter by metadata view

auto result = client.get_leaderboard(req);

if (result.is_ok()) {
    const auto& response = result.value();

    std::cout << "Period: " << response.period_start() << std::endl;
    std::cout << "Total entries: " << response.total_entries() << std::endl;

    // View info (when filtering by view_slug)
    if (response.has_view()) {
        std::cout << "Viewing: " << response.view().name() << std::endl;
    }

    for (const auto& entry : response.entries()) {
        std::cout << "#" << entry.rank() << " "
                  << entry.player_id() << " - "
                  << entry.score() << std::endl;

        // Bracket info (if enabled)
        if (entry.has_bracket()) {
            std::cout << "  Bracket: " << entry.bracket().name()
                      << " (" << entry.bracket().color() << ")" << std::endl;
        }
    }
}
```

### Getting Player Rank

```cpp
ascnd::GetPlayerRankRequest req;
req.set_leaderboard_id("high-scores");
req.set_player_id("player123");
req.set_period("current");
req.set_view_slug("na-region");  // Optional: filter by metadata view

auto result = client.get_player_rank(req);

if (result.is_ok()) {
    const auto& response = result.value();
    if (response.has_rank()) {
        std::cout << "Rank: " << response.rank() << std::endl;
        std::cout << "Score: " << response.score() << std::endl;
        std::cout << "Best score: " << response.best_score() << std::endl;
        std::cout << "Percentile: " << response.percentile() << std::endl;

        // Bracket info (if enabled)
        if (response.has_bracket()) {
            std::cout << "Bracket: " << response.bracket().name()
                      << " (" << response.bracket().color() << ")" << std::endl;
        }

        // View info (when filtering by view_slug)
        if (response.has_view()) {
            std::cout << "View: " << response.view().name() << std::endl;
        }

        // Global rank (overall rank when filtering by view)
        if (response.has_global_rank()) {
            std::cout << "Global rank: " << response.global_rank() << std::endl;
        }
    } else {
        std::cout << "Player not on leaderboard" << std::endl;
    }
}
```

### Anticheat

The SDK includes server-side anticheat validation. Check the anticheat result after submitting a score:

```cpp
auto result = client.submit_score(req);

if (result.is_ok()) {
    const auto& response = result.value();

    // Check if anticheat validation passed
    if (response.anticheat().passed()) {
        std::cout << "Score validated successfully" << std::endl;
    } else {
        std::cout << "Anticheat flagged this score" << std::endl;

        // Check for specific violations
        for (const auto& violation : response.anticheat().violations()) {
            std::cout << "Violation: " << violation.flag_type() << " - "
                      << violation.reason() << std::endl;
        }
    }
}
```

### Brackets

Players are automatically assigned to skill brackets based on their performance:

```cpp
auto result = client.get_leaderboard(req);

if (result.is_ok()) {
    for (const auto& entry : result.value().entries()) {
        std::cout << entry.player_id() << " - Bracket: "
                  << entry.bracket().name() << std::endl;

        // Get bracket color for UI display
        std::cout << "Color: " << entry.bracket().color() << std::endl;
    }
}
```

### Metadata Views

Query scores with custom metadata projections using view slugs:

```cpp
ascnd::GetLeaderboardRequest req;
req.set_leaderboard_id("high-scores");
req.set_view_slug("weekly-character-stats");

auto result = client.get_leaderboard(req);

if (result.is_ok()) {
    const auto& response = result.value();

    // Check if the view was applied
    if (response.has_view()) {
        std::cout << "Using view: " << response.view().name() << std::endl;
    }

    for (const auto& entry : response.entries()) {
        std::cout << entry.player_id() << " - "
                  << entry.score() << std::endl;
        std::cout << "Metadata: " << entry.metadata() << std::endl;
    }
}
```

### Global Rank

When filtering by a metadata view, the global rank shows the player's overall position across all views:

```cpp
// Get player rank within a view AND their global rank
ascnd::GetPlayerRankRequest req;
req.set_leaderboard_id("high-scores");
req.set_player_id("player123");
req.set_view_slug("warrior-class");  // Filter by character class

auto result = client.get_player_rank(req);

if (result.is_ok()) {
    const auto& response = result.value();
    if (response.has_rank()) {
        std::cout << "View rank: " << response.rank() << std::endl;  // Rank within warriors

        if (response.has_global_rank()) {
            std::cout << "Global rank: " << response.global_rank() << std::endl;  // Rank across all classes
        }
    }
}
```

### Async Operations

```cpp
// Non-blocking score submission
client.submit_score_async(req, [](ascnd::Result<ascnd::SubmitScoreResponse> result) {
    if (result.is_ok()) {
        // Update UI on main thread
        std::cout << "Rank: " << result.value().rank() << std::endl;
    }
});

// Non-blocking leaderboard fetch
client.get_leaderboard_async(req, [](ascnd::Result<ascnd::GetLeaderboardResponse> result) {
    if (result.is_ok()) {
        // Process leaderboard data
    }
});
```

### Error Handling

```cpp
auto result = client.submit_score(req);

if (result.is_error()) {
    std::cerr << "Error: " << result.error() << std::endl;

    switch (result.error_code()) {
        case grpc::StatusCode::UNAUTHENTICATED:
            // Invalid API key
            break;
        case grpc::StatusCode::NOT_FOUND:
            // Leaderboard not found
            break;
        case grpc::StatusCode::RESOURCE_EXHAUSTED:
            // Rate limited
            break;
        default:
            // Other error
            break;
    }
}

// Or use value_or for defaults
auto response = result.value_or(default_response);
```

## Links

- [Documentation](https://docs.ascnd.gg/sdks/cpp)
- [GitHub](https://github.com/ascnd-gg/ascnd-client-cpp)
- [Examples](https://github.com/ascnd-gg/ascnd-client-cpp/tree/main/examples)

## License

MIT License - See [LICENSE](LICENSE) for details.

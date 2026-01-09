# ascnd-client Changelog

## 1.1.1

### Fixed

- Re-release to correct publishing issue with v1.1.0

## 1.0.0

### Initial Release

- Initial release of `ascnd-client`
- Migrated to new GitHub organization (ascnd-gg)
- Full C++17 support with header-only and static library options
- Thread-safe operations

### Features

- `submit_score()` - Submit player scores to leaderboards
- `get_leaderboard()` - Retrieve top scores with pagination
- `get_player_rank()` - Get a specific player's rank and percentile
- Async versions of all methods with callbacks
- Automatic retry logic with exponential backoff
- Result type for safe error handling
- Configurable timeouts and retry settings

# Metadata & Periods Example

Demonstrates submitting scores with metadata and querying by time period.

## Build (Standalone)

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Build (Within SDK)

From the SDK root:

```bash
mkdir build && cd build
cmake .. -DASCND_BUILD_EXAMPLES=ON
cmake --build .
```

## Environment Variables

| Variable | Description |
|----------|-------------|
| `ASCND_API_KEY` | Your Ascnd API key |
| `LEADERBOARD_ID` | Target leaderboard ID |

## Run

```bash
export ASCND_API_KEY=your_api_key
export LEADERBOARD_ID=your_leaderboard_id
./metadata-periods
```

## Expected Output

```
Score submitted with metadata! Rank: #3

Current Period: 2024-01-01T00:00:00Z
Ends: 2024-01-08T00:00:00Z

Top 5 with metadata:

#1 player_champion: 99000
   Metadata: {"character":"mage","level":20}
#2 player_silver: 85000
#3 player_meta_001: 75000
   Metadata: {"character":"warrior","level":15,"powerups":["speed","shield"]}
...
```

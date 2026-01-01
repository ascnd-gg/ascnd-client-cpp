# Leaderboard Example

Demonstrates fetching and displaying a leaderboard from Ascnd.

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
./leaderboard
```

## Expected Output

```
Top 10 Leaderboard (1523 total players)

Rank  | Player             | Score
------+--------------------+------------
   1  | player_champion    |     999999
   2  | player_silver      |     875000
   3  | player_bronze      |     750000
...
```

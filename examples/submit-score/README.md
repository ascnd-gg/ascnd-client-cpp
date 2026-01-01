# Submit Score Example

Demonstrates submitting a score to an Ascnd leaderboard.

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
./submit-score
```

## Expected Output

```
Score submitted!
  Rank: #42
  New personal best: Yes!
```

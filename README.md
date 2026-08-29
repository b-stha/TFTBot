# Mittens

Mittens is a C++ Discord bot that watches tracked League of Legends: Teamfight Tactics players, polls Riot's API for new matches, and posts match summaries directly into Discord channels.

It is built to be a lightweight, low-ceremony tracking bot for a small set of players rather than a large multi-tenant analytics platform.

## What it does

- tracks a player by Riot PUUID and Discord channel
- stores the selected TFT queue types for each tracked player
- polls Riot for new matches and suppresses duplicate reposts
- restores tracked players across restarts using SQLite persistence
- posts ranked, double-up, and normal-match embeds with player stats and board details
- shows match outcome, placement, level, gold, board value, and unit/trait summaries

## Why it exists

Mittens is designed for quick personal match tracking in Discord. Instead of building a broad dashboard or external service, it keeps the runtime model simple: load tracked players, watch for new match results, and post a result when something new is found.

## Architecture

The project is organized around a few core pieces:

- `Bot` owns the Discord cluster and command registration
- `Worker` manages queued player processing asynchronously
- `Player` stores tracked runtime state for a user
- `RiotAPI` handles the Riot HTTP calls
- `Data` loads static game data like unit and trait metadata
- `Persistence` stores tracked-player state in SQLite for restart recovery

The runtime model remains in-memory. SQLite is used for persistence, not as a replacement runtime layer.

## Technologies

- C++17
- DPP for Discord integration
- Riot Games API via async HTTP requests
- SQLite for durable tracking state
- CMake for build configuration
- nlohmann/json for API JSON parsing
- OpenSSL for platform dependencies

## Player tracking workflow

When a user adds a Riot account through the `/add` command:

1. the bot resolves the Riot account by name/tag
2. a `Player` object is created or restored
3. the queue selection is added to the player
4. Riot data is fetched for league state and match results
5. the bot stores the tracking state in SQLite
6. the worker queue processes future match updates asynchronously

## Supported functionality

- add players by `name#tag`
- track ranked, normal, double-up, or all queues
- restore tracking state after a restart
- suppress duplicate match posts using the current/pending match IDs
- send Discord embeds with match summary information
- update ranked promotion notifications when rank changes

## Build requirements

- CMake 3.22+
- C++17 compiler
- OpenSSL development libraries
- DPP library
- nlohmann JSON
- SQLite is fetched and built via CMake automatically

## Build and run

From the project root:

```bash
cmake -S . -B build
cmake --build build
```

Then run the bot with the required environment variables set:

```bash
export BOT_TOKEN="your_discord_bot_token"
export TFT_APIKEY="your_riot_api_key"
./build/Mittens
```

## Required environment variables

- `BOT_TOKEN`: Discord bot token
- `TFT_APIKEY`: Riot API key

## SQLite behavior

The bot creates a local SQLite database named `mittens.db` in the working directory when the persistence layer is initialized. It stores only the minimal player tracking state needed to restore the app after a restart, including:

- Riot PUUID
- Riot username
- Riot tag
- Discord channel ID
- selected queue IDs
- current match ID used for duplicate prevention

No Discord credentials or large Riot payloads are stored.

## Local development notes

This project keeps generated build output and local task notes out of version control via `.gitignore`.

## Example Discord output

<img width="561" height="647" alt="image" src="https://github.com/user-attachments/assets/f5ce85e9-6cbc-41cd-9c00-c8fa8287f0cb" />

The bot sends embed cards showing placement, rank, board value, unit and trait summaries, and a direct link to the match result page.

## Final status

This project is a focused C++ Discord bot for TFT match tracking, with startup persistence, queued async polling, and a small automated test target covering deterministic helper logic.

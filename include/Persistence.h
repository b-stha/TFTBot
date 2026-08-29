/*
Class Persistence manages saving/loading tracked players to SQLite.
*/

#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include <cstdint>
#include <string>
#include <vector>

struct sqlite3;

struct PersistedPlayer {
	std::string puuid;
	std::string name;
	std::string tag;
	std::uint64_t channelID = 0;
	std::vector<int> queueIDs;
	std::string currentMatchID;
};

class Persistence {
public:
	explicit Persistence(const std::string& databasePath);
	~Persistence();

	Persistence(const Persistence&) = delete;
	Persistence& operator=(const Persistence&) = delete;

	bool isOpen() const;
	bool savePlayer(const PersistedPlayer& player);
	bool removePlayer(const std::string& puuid);
	std::vector<PersistedPlayer> loadPlayers();

private:
	sqlite3* database = nullptr;
};

#endif

#include "Persistence.h"

#include <sqlite3.h>
#include <iostream>
#include <mutex>

namespace {
std::mutex databaseMutex;

void logDatabaseError(const char* operation, sqlite3* database) {
	std::cerr << "[Persistence] " << operation << " failed: "
		<< (database != nullptr ? sqlite3_errmsg(database) : "database unavailable") << std::endl;
}

bool bindText(sqlite3_stmt* statement, int index, const std::string& value) {
	if (statement == nullptr) {
		return false;
	}

	int rc = sqlite3_bind_text(statement, index, value.c_str(), -1, SQLITE_TRANSIENT);
	return rc == SQLITE_OK;
}

} // namespace

Persistence::Persistence(const std::string& databasePath) {
	if (sqlite3_open(databasePath.c_str(), &database) != SQLITE_OK) {
		logDatabaseError("open", database);
		sqlite3_close(database);
		database = nullptr;
		return;
	}

	const char* schema = R"SQL(
CREATE TABLE IF NOT EXISTS players (
    puuid TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    tag TEXT NOT NULL,
    channel_id INTEGER NOT NULL,
    current_match_id TEXT NOT NULL
);
CREATE TABLE IF NOT EXISTS player_queues (
    puuid TEXT NOT NULL REFERENCES players(puuid) ON DELETE CASCADE,
    queue_id INTEGER NOT NULL,
    PRIMARY KEY (puuid, queue_id)
);
PRAGMA foreign_keys = ON;
)SQL";
	char* errorMessage = nullptr;
	int rc = sqlite3_exec(database, schema, nullptr, nullptr, &errorMessage);
	if (rc != SQLITE_OK) {
		std::cerr << "[Persistence] schema initialization failed: "
			<< (errorMessage != nullptr ? errorMessage : "unknown error") << std::endl;
		sqlite3_free(errorMessage);
		sqlite3_close(database);
		database = nullptr;
	}
}

Persistence::~Persistence() {
	if (database != nullptr) {
		sqlite3_close(database);
	}
}

bool Persistence::isOpen() const {
	return database != nullptr;
}

bool Persistence::savePlayer(const PersistedPlayer& player) {
	std::lock_guard<std::mutex> lock(databaseMutex);
	if (database == nullptr || player.puuid.empty()) {
		return false;
	}

	char* errorMessage = nullptr;
	int rc = sqlite3_exec(database, "BEGIN TRANSACTION", nullptr, nullptr, &errorMessage);
	if (rc != SQLITE_OK) {
		std::cerr << "[Persistence] save transaction failed: " << errorMessage << std::endl;
		sqlite3_free(errorMessage);
		return false;
	}

	const char* playerSQL = "INSERT INTO players (puuid, name, tag, channel_id, current_match_id) VALUES (?, ?, ?, ?, ?) "
		"ON CONFLICT(puuid) DO UPDATE SET name=excluded.name, tag=excluded.tag, channel_id=excluded.channel_id, current_match_id=excluded.current_match_id";
	sqlite3_stmt* playerStatement = nullptr;
	bool success = false;

	rc = sqlite3_prepare_v2(database, playerSQL, -1, &playerStatement, nullptr);
	if (rc == SQLITE_OK) {
		rc = bindText(playerStatement, 1, player.puuid);
		if (rc) {
			rc = bindText(playerStatement, 2, player.name);
		}
		if (rc) {
			rc = bindText(playerStatement, 3, player.tag);
		}
		if (rc) {
			rc = sqlite3_bind_int64(playerStatement, 4, static_cast<sqlite3_int64>(player.channelID));
		}
		if (rc) {
			rc = bindText(playerStatement, 5, player.currentMatchID);
		}
		if (rc) {
			rc = sqlite3_step(playerStatement);
		}
		if (rc == SQLITE_DONE) {
			success = true;
		}
	}
	if (playerStatement != nullptr) {
		sqlite3_finalize(playerStatement);
	}

	if (success) {
		const char* deleteSQL = "DELETE FROM player_queues WHERE puuid = ?";
		sqlite3_stmt* queueDeleteStatement = nullptr;
		rc = sqlite3_prepare_v2(database, deleteSQL, -1, &queueDeleteStatement, nullptr);
		if (rc == SQLITE_OK) {
			rc = bindText(queueDeleteStatement, 1, player.puuid);
			if (rc) {
				rc = sqlite3_step(queueDeleteStatement);
			}
		}
		if (queueDeleteStatement != nullptr) {
			sqlite3_finalize(queueDeleteStatement);
		}
		if (rc != SQLITE_DONE) {
			success = false;
		}
	}

	if (success) {
		for (int queueID : player.queueIDs) {
			const char* queueSQL = "INSERT INTO player_queues (puuid, queue_id) VALUES (?, ?)";
			sqlite3_stmt* queueStatement = nullptr;
			rc = sqlite3_prepare_v2(database, queueSQL, -1, &queueStatement, nullptr);
			if (rc != SQLITE_OK) {
				success = false;
				break;
			}

			rc = bindText(queueStatement, 1, player.puuid);
			if (rc) {
				rc = sqlite3_bind_int(queueStatement, 2, queueID);
			}
			if (rc) {
				rc = sqlite3_step(queueStatement);
			}
			if (queueStatement != nullptr) {
				sqlite3_finalize(queueStatement);
			}
			if (rc != SQLITE_DONE) {
				success = false;
				break;
			}
		}
	}

	if (success) {
		rc = sqlite3_exec(database, "COMMIT", nullptr, nullptr, &errorMessage);
		if (rc != SQLITE_OK) {
			success = false;
		}
	} else {
		sqlite3_exec(database, "ROLLBACK", nullptr, nullptr, nullptr);
		logDatabaseError("save", database);
	}

	sqlite3_free(errorMessage);
	return success;
}

bool Persistence::removePlayer(const std::string& puuid) {
	std::lock_guard<std::mutex> lock(databaseMutex);
	if (database == nullptr || puuid.empty()) {
		return false;
	}

	const char* sql = "DELETE FROM players WHERE puuid = ?";
	sqlite3_stmt* statement = nullptr;
	int rc = sqlite3_prepare_v2(database, sql, -1, &statement, nullptr);
	if (rc != SQLITE_OK) {
		logDatabaseError("delete", database);
		return false;
	}

	rc = bindText(statement, 1, puuid);
	if (rc) {
		rc = sqlite3_step(statement);
	}
	if (statement != nullptr) {
		sqlite3_finalize(statement);
	}
	if (rc != SQLITE_DONE) {
		logDatabaseError("delete", database);
		return false;
	}

	return true;
}

std::vector<PersistedPlayer> Persistence::loadPlayers() {
	std::lock_guard<std::mutex> lock(databaseMutex);
	std::vector<PersistedPlayer> players;
	if (database == nullptr) {
		return players;
	}

	const char* sql = "SELECT puuid, name, tag, channel_id, current_match_id FROM players";
	sqlite3_stmt* statement = nullptr;
	int rc = sqlite3_prepare_v2(database, sql, -1, &statement, nullptr);
	if (rc != SQLITE_OK) {
		logDatabaseError("load", database);
		return players;
	}

	while (true) {
		rc = sqlite3_step(statement);
		if (rc == SQLITE_ROW) {
			PersistedPlayer player;
			player.puuid = reinterpret_cast<const char*>(sqlite3_column_text(statement, 0));
			player.name = reinterpret_cast<const char*>(sqlite3_column_text(statement, 1));
			player.tag = reinterpret_cast<const char*>(sqlite3_column_text(statement, 2));

			const char* queueSQL = "SELECT queue_id FROM player_queues WHERE puuid = ?";
			sqlite3_stmt* queueStatement = nullptr;
			int queueRC = sqlite3_prepare_v2(database, queueSQL, -1, &queueStatement, nullptr);
			if (queueRC == SQLITE_OK) {
				queueRC = bindText(queueStatement, 1, player.puuid);
				if (queueRC) {
					while (true) {
						queueRC = sqlite3_step(queueStatement);
						if (queueRC == SQLITE_ROW) {
							player.queueIDs.push_back(sqlite3_column_int(queueStatement, 0));
							continue;
						}
						break;
					}
				}
			}
			if (queueStatement != nullptr) {
				sqlite3_finalize(queueStatement);
			}

			player.channelID = static_cast<std::uint64_t>(sqlite3_column_int64(statement, 3));
			player.currentMatchID = reinterpret_cast<const char*>(sqlite3_column_text(statement, 4));
			players.push_back(std::move(player));
			continue;
		}
		break;
	}

	if (statement != nullptr) {
		sqlite3_finalize(statement);
	}
	return players;
}

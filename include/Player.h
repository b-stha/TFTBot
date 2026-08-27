/*
Class Player that stores all of the relevant information for a player that is needed to create Discord embeds.
Includes League declaration for storing ranked information.

*/

#ifndef PLAYER_H
#define PLAYER_H

#include <string>
#include <vector>
#include <dpp/dpp.h>
#include "MatchInfo.h"
#include <unordered_set>
#include <mutex>

struct League {
	std::string tier = "UNRANKED";
	std::string prevTier = "UNRANKED";
	int prevLP = 0;
	int currLP = 0;
	std::string rank = "";
};

class Player {
public:
	Player(std::string inputPuuid);
	// getter functions
	std::string getCurrMatchID() const;
	std::string getPendingMatchID() const;
	std::string getPUUID() const;
	std::vector<int> getTime() const;
	std::string getSummonerID() const; // obsolete, can be removed (?)
	std::vector<std::string> getFullName() const;
	std::pair<std::string, std::string> getRank() const;
	std::pair<std::string, std::string> getDoubleUpRank() const;
	dpp::snowflake getChannelID() const;
	std::vector<League> getAllRanks() const;
	std::pair<int, int> getRankedLP() const;
	std::pair<int, int> getDoubleUpLP() const;
	std::string getPrevRanked() const;
	std::string getPrevDoubleUp() const;
	MatchInfo getMatchInfo() const;
	std::unordered_set<int> getAddedQueues() const;
	// setter functions
	void setNameTag(std::string inputName, std::string inputTag);
	void setPrevMatch(std::string matchID);
	void setCurrMatch(std::string matchID);
	void setPendingMatchID(std::string matchID);
	void setChannelID(dpp::snowflake inputChannelID);
	void setSummonerID(std::string summonerID); // obsolete, can be removed (?)
	void setPlayerLeague(const std::vector<League>& inLeague);
	void setMatchInfo(const MatchInfo& currMatch);

	void updateRankedLP(const int newLP);
	void updateDoubleUpLP(const int newLP);
	void updateRankedTier(const std::string newTier, const std::string newRank);
	void updateDoubleUpTier(const std::string newTier, const std::string newRank);
	void addQueue(int queueID);
private:
	mutable std::mutex playerMutex;
	std::unordered_set<int> addedQueues; // queue IDs that user has added, determines which embeds to create
	MatchInfo matchInfo;
	dpp::snowflake channelID;
	std::string puuid;
	std::string prevMatchID = "";
	std::string currMatchID = "";
	std::string pendingMatchID = "";
	std::string userName = "";
	std::string tagLine = "";
	std::string summonerID; // obsolete, can be removed (?)
	std::vector<League> leagues; // index 0 is ranked, index 1 is double up
};

#endif
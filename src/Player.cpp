#include "Player.h"
#include <iostream>
#include <fstream>
#include <cmath>

Player::Player(std::string inputPuuid) {
	std::lock_guard<std::mutex> lock(playerMutex);
	puuid = inputPuuid;
}

std::string Player::getPUUID() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return puuid;
};

void Player::setNameTag(std::string inputName, std::string inputTag) {
	std::lock_guard<std::mutex> lock(playerMutex);
	userName = inputName;
	tagLine = inputTag;
};

void Player::setPrevMatch(std::string matchID) {
	std::lock_guard<std::mutex> lock(playerMutex);
	prevMatchID = matchID;
};

void Player::setCurrMatch(std::string matchID) {
	std::lock_guard<std::mutex> lock(playerMutex);
	currMatchID = matchID;
};

void Player::setPendingMatchID(std::string matchID) {
	std::lock_guard<std::mutex> lock(playerMutex);
	pendingMatchID = matchID;
};

std::string Player::getCurrMatchID() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return currMatchID;
};

std::string Player::getPendingMatchID() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return pendingMatchID;
};

std::vector<std::string> Player::getFullName() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	std::vector<std::string> nameVec{ userName, tagLine };
	return nameVec;
}

std::vector<int> Player::getTime() const {
	MatchInfo matchInfoCopy = getMatchInfo();
    std::cout << matchInfoCopy.gameLenMin << ":" << matchInfoCopy.gameLenSec << std::endl;
	int min = static_cast<int>(matchInfoCopy.gameLenMin);
	int sec = static_cast<int>(matchInfoCopy.gameLenSec);
	std::vector<int> timeVec{ min, sec };
	return timeVec;
}

dpp::snowflake Player::getChannelID() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return channelID;
}

void Player::setChannelID(dpp::snowflake inputChannelID) {
	std::lock_guard<std::mutex> lock(playerMutex);
	channelID = inputChannelID;
}

void Player::setSummonerID(std::string input) {
	std::lock_guard<std::mutex> lock(playerMutex);
	summonerID = input;
}

std::string Player::getSummonerID() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return summonerID;
}

void Player::setPlayerLeague(const std::vector<League>& inLeague) {
	std::lock_guard<std::mutex> lock(playerMutex);
	leagues = inLeague;
}

void Player::updateRankedLP(const int newLP) {
	std::lock_guard<std::mutex> lock(playerMutex);
	leagues[0].prevLP = leagues[0].currLP;
	leagues[0].currLP = newLP;
}

void Player::updateDoubleUpLP(const int newLP) {
	std::lock_guard<std::mutex> lock(playerMutex);
	leagues[1].prevLP = leagues[1].currLP;
	leagues[1].currLP = newLP;
}

void Player::updateRankedTier(const std::string newTier, const std::string newRank) {
	std::lock_guard<std::mutex> lock(playerMutex);
	leagues[0].prevTier = leagues[0].tier;
	leagues[0].tier = newTier;
	leagues[0].rank = newRank;
}

void Player::updateDoubleUpTier(const std::string newTier, const std::string newRank) {
	std::lock_guard<std::mutex> lock(playerMutex);
	leagues[1].prevTier = leagues[1].tier;
	leagues[1].tier = newTier;
	leagues[1].rank = newRank;
}

std::pair<std::string, std::string> Player::getRank() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return std::make_pair(leagues[0].tier, leagues[0].rank);
}

std::pair<std::string, std::string> Player::getDoubleUpRank() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return std::make_pair(leagues[1].tier, leagues[1].rank);
}

std::vector<League> Player::getAllRanks() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return leagues;
}

std::pair<int,int> Player::getRankedLP() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return std::make_pair(leagues[0].prevLP, leagues[0].currLP);
}

std::pair<int,int> Player::getDoubleUpLP() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return std::make_pair(leagues[1].prevLP, leagues[1].currLP);
}

std::string Player::getPrevRanked() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return leagues[0].prevTier;
}

std::string Player::getPrevDoubleUp() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return leagues[1].prevTier;
}

MatchInfo Player::getMatchInfo() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return matchInfo;
}

void Player::setMatchInfo(const MatchInfo& currMatch) {
	std::lock_guard<std::mutex> lock(playerMutex);
	matchInfo = currMatch;
}

std::unordered_set<int> Player::getAddedQueues() const {
	std::lock_guard<std::mutex> lock(playerMutex);
	return addedQueues;
}

void Player::addQueue(int queueID) {
	std::lock_guard<std::mutex> lock(playerMutex);
	addedQueues.insert(queueID);
}
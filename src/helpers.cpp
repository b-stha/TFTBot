#include "Player.h"
#include "helpers.h"
#include <vector>
#include <string>
#include <sstream>
#include "data.h"

std::string operator * (std::string a, unsigned int b) {
	std::string output = "";
	while (b--) {
		output += a;
	}
	return output;
}

std::string setStrWidth(const std::string& str, int len) {
	if (len <= 0) {
		return "";
	}

	if (static_cast<int>(str.length()) >= len) {
		return str.substr(0, static_cast<size_t>(len));
	}

	std::string result = str;
	int spaces_to_add = len - static_cast<int>(str.length());
	for (int i = 0; i < spaces_to_add; ++i) {
		result += " ";
	}
	return result;
}

std::string starCount(const int& tier) {
	std::string star = ":star:";
	return (star * tier);
};

std::vector<std::string> split(const std::string& s, char delim) {
	std::vector<std::string> result;
	std::stringstream ss(s);
	std::string item;

	while (getline(ss, item, delim)) {
		result.push_back(item);
	}

	return result;
}

bool notPlayerExists(const std::vector<std::shared_ptr<Player>>& players, const std::string& puuid) {
	return std::none_of(players.begin(), players.end(),
		[&puuid](const std::shared_ptr<Player>& player) {
			return player->getPUUID() == puuid;
		});
}

std::string fillSpaces(std::string input) {
    std::string fillStr = "%20";
    for (size_t i = 0; i < input.length(); i++) {
        if (input[i] == ' ') {
            input.erase(i,1);
            input.insert(i, fillStr);
        }
    }
    return input;
}

std::string lowerCase(std::string str) {
	std::transform(str.begin(), str.end(), str.begin(), ::tolower);
	return str;
}

std::string getRankField(const Player& player, const Data& data, std::string queueType) {
	std::string playerTier = "";
	if (queueType == "DOUBLE_UP") {
		player.getDoubleUpRank().first;
		if (playerTier == "UNRANKED") {
			return data.getEmote(lowerCase("UNRANKED")) + " UNRANKED ";
		}
		playerTier = player.getDoubleUpRank().first;
		return data.getEmote(lowerCase(playerTier)) + " " + playerTier + " " + player.getDoubleUpRank().second + " (" + std::to_string(player.getDoubleUpLP().second) + " LP)";
	}
	else if (queueType == "RANKED") {
		playerTier = player.getRank().first;
		if (playerTier == "UNRANKED") {
			return data.getEmote(lowerCase("UNRANKED")) + " UNRANKED ";
		}
		playerTier = player.getRank().first;
		
		if (playerTier == "MASTER" || playerTier == "GRANDMASTER" || playerTier == "CHALLENGER") {
			return data.getEmote(lowerCase(playerTier)) + " " + playerTier + " (" + std::to_string(player.getRankedLP().second) + " LP)";
		}
		return data.getEmote(lowerCase(playerTier)) + " " + playerTier + " " + player.getRank().second + " (" + std::to_string(player.getRankedLP().second) + " LP)";
	}

	return "Error displaying rank.";
}

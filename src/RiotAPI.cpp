#include "RiotAPI.h"
#include "Player.h"
#include <nlohmann/json.hpp>
#include <string>
#include "helpers.h"

using json = nlohmann::json;

Riot::Riot(dpp::cluster& bot, const std::string& apiKey)
	: botCluster(bot), apiKey(apiKey)
	{}

void Riot::fetchMatchID(std::shared_ptr<Player> pPlayer, std::function<void(bool)> next) {
	std::string matchIDurl = "https://americas.api.riotgames.com/tft/match/v1/matches/by-puuid/" + pPlayer->getPUUID() + "/ids?count=1&api_key=" + apiKey;
	botCluster.request(matchIDurl, dpp::m_get, [pPlayer, next, matchIDurl](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			std::cout <<"Match ID HTTP request failed with status: " + std::to_string(http.status) << std::endl;
			std::cout << "url: " << matchIDurl << std::endl;
			std::cout << "body: " << http.body << std::endl;
			if (next) next(false);
			return;
		}

		try {
			std::cout << "Request URL: " << matchIDurl << "\n";
			std::cout << "HTTP Status: " << http.status << "\n";
			std::cout << "HTTP Body: " << http.body << "\n";
			json matchIDJson = json::parse(http.body);

			if (!matchIDJson.is_array() || matchIDJson.empty()) {
				std::cout << "No matches found for player with PUUID: " + pPlayer->getPUUID() << std::endl;
				if (next) next(false);
				return;
			}

			std::string latestMatchID = matchIDJson[0].get<std::string>();
			if (latestMatchID == pPlayer->getCurrMatchID()) {
				std::cout << "No new match found for player with PUUID: " + pPlayer->getPUUID() << std::endl;
				if (next) next(false);
				return;
			}

			pPlayer->setPrevMatch(pPlayer->getCurrMatchID());
			pPlayer->setCurrMatch(latestMatchID);
			if (next) next(true);
		} catch (const json::exception& e) {
			// Covers parse failures and any missing/malformed fields in the response.
			std::cerr << "JSON error for url:" << matchIDurl << " - " << e.what() << "\nbody: " << http.body << std::endl;
			if (next) next(false);
		}
	});
}

void Riot::fetchInfo(std::shared_ptr<Player> pPlayer, std::function<void(bool)> next) {
	std::string infoURL = "https://americas.api.riotgames.com/tft/match/v1/matches/" + pPlayer->getCurrMatchID() + "?api_key=" + apiKey;
    botCluster.request(infoURL, dpp::m_get, [pPlayer, next, infoURL](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			std::cout <<"Match info HTTP request failed with status: " + std::to_string(http.status) << std::endl;
			std::cout << "url: " << infoURL << std::endl;
			std::cout << "body: " << http.body << std::endl;
			if (next) next(false);
			return;
		}

		try {
			std::cout << "Request URL: " << infoURL << "\n";
			std::cout << "HTTP Status: " << http.status << "\n";
			std::cout << "HTTP Body: " << http.body << "\n";
			json matchJson = json::parse(http.body);

			const json& allInfo = matchJson.at("info");
			const json& participants = allInfo.at("participants");

			bool found = false;
			MatchInfo matchInfo;
			for (const json& participant : participants) {
				if (pPlayer->getPUUID() == participant.at("puuid").get<std::string>()) {
					matchInfo = participant.get<MatchInfo>();
					double totalSeconds = participant.at("time_eliminated").get<double>();
					matchInfo.gameLenMin = static_cast<int>(totalSeconds / 60);
					matchInfo.gameLenSec = static_cast<int>(totalSeconds) % 60;
					matchInfo.boardValue = matchInfo.calcBoardValue();
					matchInfo.queueID = allInfo.at("queue_id").get<int>();
					found = true;
					break;
				}
			}

			if (!found) {
				// Player not present in the match response counts as invalid data, not success.
				std::cout << "Player with PUUID: " << pPlayer->getPUUID() << " not found in match participants." << std::endl;
				if (next) next(false);
				return;
			}

			pPlayer->setMatchInfo(matchInfo);
			if (next) next(true);
		} catch (const json::exception& e) {
			std::cerr << "JSON error for url:" << infoURL << " - " << e.what() << "\nbody: " << http.body << std::endl;
			if (next) next(false);
		}
	});
};

void Riot::setName(std::shared_ptr<Player> pPlayer) {
	std::string nameURL = "https://americas.api.riotgames.com/riot/account/v1/accounts/by-puuid/" + pPlayer->getPUUID() + "?api_key=" + apiKey;
	botCluster.request(nameURL, dpp::m_get, [pPlayer, nameURL](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			std::cout <<"Player name HTTP request failed with status: " + std::to_string(http.status) << std::endl;
			std::cout << "url: " << nameURL << std::endl;
			std::cout << "body: " << http.body << std::endl;
			return;
		}

		json nameJson;
		try {
			std::cout << "Request URL: " << nameURL << "\n";
			std::cout << "HTTP Status: " << http.status << "\n";
			std::cout << "HTTP Body: " << http.body << "\n";
			nameJson = json::parse(http.body);
		} catch (const json::parse_error& e) {
			std::cerr << "JSON parse error for url:" << nameURL << " - " << e.what() << "\nbody: " << http.body << std::endl;
			return;
		}

		std::string name = nameJson["gameName"].get<std::string>();
		std::string tag = nameJson["tagLine"].get<std::string>();
		pPlayer->setNameTag(name, tag);
	});
}

void Riot::fetchPUUID(const std::string& name, const std::string& tag, std::function<void(const std::string&)> next) {
    std::string fixedName = fillSpaces(name);
	std::string idURL = "https://americas.api.riotgames.com/riot/account/v1/accounts/by-riot-id/" + fixedName + "/" + tag + "?api_key=" + apiKey;
    botCluster.request(idURL, dpp::m_get, [name, tag, next, idURL](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			std::cout <<"PUUID HTTP request failed with status: " + std::to_string(http.status) << std::endl;
			std::cout << "url: " << idURL << std::endl;
			std::cout << "body: " << http.body << std::endl;
			if (next) next("");
			return;
		}

		try {
			std::cout << "Request URL: " << idURL << "\n";
			std::cout << "HTTP Status: " << http.status << "\n";
			std::cout << "HTTP Body: " << http.body << "\n";
			json idJson = json::parse(http.body);
			std::string puuid = idJson.at("puuid").get<std::string>();
			std::cout << "puuid: " << puuid << std::endl;
			if (next) next(puuid);
		} catch (const json::exception& e) {
			std::cerr << "JSON error for url:" << idURL << " - " << e.what() << "\nbody: " << http.body << std::endl;
			if (next) next("");
		}
	});
}

void Riot::fetchSummonerID(std::shared_ptr<Player> pPlayer) {
	std::string summonerURL = "https://na1.api.riotgames.com/tft/summoner/v1/summoners/by-puuid/" + pPlayer->getPUUID() + "?api_key=" + apiKey;
	botCluster.request(summonerURL, dpp::m_get, [pPlayer, summonerURL](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			std::cout <<"Summoner ID HTTP request failed with status: " + std::to_string(http.status) << std::endl;
			std::cout << "url: " << summonerURL << std::endl;
			std::cout << "body: " << http.body << std::endl;
			return;
		}

		json summonerJson;
		try {
			
			std::cout << "Request URL: " << summonerURL << "\n";
			std::cout << "HTTP Status: " << http.status << "\n";
			std::cout << "HTTP Body: " << http.body << "\n";
			summonerJson = json::parse(http.body);
		} catch (const json::parse_error& e) {
			std::cerr << "JSON parse error for url:" << summonerURL << " - " << e.what() << "\nbody: " << http.body << std::endl;
			return;
		}

		std::string summonerID = summonerJson["id"].get<std::string>();
		pPlayer->setSummonerID(summonerID);
	});
}

void Riot::fetchLeague(std::shared_ptr<Player> pPlayer, std::function<void(bool)> next) {
	std::string leagueURL = "https://na1.api.riotgames.com/tft/league/v1/by-puuid/" + pPlayer->getPUUID() + "?api_key=" + apiKey;
	botCluster.request(leagueURL, dpp::m_get, [pPlayer, next, leagueURL](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			std::cout <<"League HTTP request failed with status: " + std::to_string(http.status) << std::endl;
			std::cout << "url: " << leagueURL << std::endl;
			std::cout << "body: " << http.body << std::endl;
			if (next) next(false);
			return;
		}

		try {
			std::cout << "Request URL: " << leagueURL << "\n";
			std::cout << "HTTP Status: " << http.status << "\n";
			std::cout << "HTTP Body: " << http.body << "\n";
			json leagueJson = json::parse(http.body);
			std::cout << leagueJson.dump(2) << std::endl;

			if (!leagueJson.is_array() || leagueJson.empty()) {
				std::cout << "No league data found for player with PUUID: " + pPlayer->getPUUID() << std::endl;
				if (next) next(false);
				return;
			}

			if (pPlayer->getAllRanks().empty()) {
				std::vector<League> initLeagues(2);
				pPlayer->setPlayerLeague(initLeagues);
			}

			for (const json& leagueEntry : leagueJson) {
				std::string queueType = leagueEntry.at("queueType").get<std::string>();
				if (queueType == "RANKED_TFT_DOUBLE_UP") {
					pPlayer->updateDoubleUpLP(leagueEntry.at("leaguePoints").get<int>());
					pPlayer->updateDoubleUpTier(leagueEntry.at("tier").get<std::string>(), leagueEntry.at("rank").get<std::string>());
				}
				else if (queueType == "RANKED_TFT") {
					pPlayer->updateRankedLP(leagueEntry.at("leaguePoints").get<int>());
					pPlayer->updateRankedTier(leagueEntry.at("tier").get<std::string>(), leagueEntry.at("rank").get<std::string>());
				}
			}

			if (next) next(true);
		} catch (const json::exception& e) {
			std::cerr << "JSON error for url:" << leagueURL << " - " << e.what() << "\nbody: " << http.body << std::endl;
			if (next) next(false);
		}
	});
}

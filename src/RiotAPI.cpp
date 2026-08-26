#include "RiotAPI.h"
#include "Player.h"
#include <nlohmann/json.hpp>
#include <string>
#include "helpers.h"

using json = nlohmann::json;

namespace {

// Riot API keys are passed as a URL query param; never let them reach the logs.
std::string redactApiKey(const std::string& url) {
	auto pos = url.find("api_key=");
	if (pos == std::string::npos) {
		return url;
	}
	return url.substr(0, pos) + "api_key=REDACTED";
}

std::string truncate(const std::string& s, size_t maxLen = 200) {
	return s.size() > maxLen ? s.substr(0, maxLen) + "...(truncated)" : s;
}

void logHttpError(const std::string& context, const std::string& url, const dpp::http_request_completion_t& http) {
	const std::string safeUrl = redactApiKey(url);
	if (http.status == 401 || http.status == 403) {
		std::cerr << "[Riot API] " << context << ": auth error (status " << http.status << "), check API key. url: " << safeUrl << std::endl;
	} else if (http.status == 429) {
		std::cerr << "[Riot API] " << context << ": rate limited, retry after " << http.ratelimit_retry_after << "s. url: " << safeUrl << std::endl;
	} else if (http.status == 404) {
		std::cerr << "[Riot API] " << context << ": not found (404). url: " << safeUrl << std::endl;
	} else if (http.status >= 500) {
		std::cerr << "[Riot API] " << context << ": Riot server error (status " << http.status << "). url: " << safeUrl << std::endl;
	} else {
		std::cerr << "[Riot API] " << context << ": unexpected status " << http.status << ". url: " << safeUrl << ", body: " << truncate(http.body) << std::endl;
	}
}

void logMalformedResponse(const std::string& context, const std::string& url, const std::string& what, const std::string& body) {
	std::cerr << "[Riot API] " << context << ": malformed response - " << what << ". url: " << redactApiKey(url) << ", body: " << truncate(body) << std::endl;
}

} // namespace

Riot::Riot(dpp::cluster& bot, const std::string& apiKey)
	: botCluster(bot), apiKey(apiKey)
	{}

void Riot::fetchMatchID(std::shared_ptr<Player> pPlayer, std::function<void(bool)> next) {
	std::string matchIDurl = "https://americas.api.riotgames.com/tft/match/v1/matches/by-puuid/" + pPlayer->getPUUID() + "/ids?count=1&api_key=" + apiKey;
	botCluster.request(matchIDurl, dpp::m_get, [pPlayer, next, matchIDurl](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			logHttpError("fetchMatchID", matchIDurl, http);
			if (next) next(false);
			return;
		}

		try {
			json matchIDJson = json::parse(http.body);

			if (!matchIDJson.is_array() || matchIDJson.empty()) {
				std::cout << "[Riot API] fetchMatchID: no matches found for PUUID " << pPlayer->getPUUID() << std::endl;
				if (next) next(false);
				return;
			}

			std::string latestMatchID = matchIDJson[0].get<std::string>();
			if (latestMatchID == pPlayer->getCurrMatchID()) {
				if (next) next(false);
				return;
			}

			pPlayer->setPrevMatch(pPlayer->getCurrMatchID());
			pPlayer->setCurrMatch(latestMatchID);
			if (next) next(true);
		} catch (const json::exception& e) {
			// Covers parse failures and any missing/malformed fields in the response.
			logMalformedResponse("fetchMatchID", matchIDurl, e.what(), http.body);
			if (next) next(false);
		}
	});
}

void Riot::fetchInfo(std::shared_ptr<Player> pPlayer, std::function<void(bool)> next) {
	std::string infoURL = "https://americas.api.riotgames.com/tft/match/v1/matches/" + pPlayer->getCurrMatchID() + "?api_key=" + apiKey;
    botCluster.request(infoURL, dpp::m_get, [pPlayer, next, infoURL](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			logHttpError("fetchInfo", infoURL, http);
			if (next) next(false);
			return;
		}

		try {
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
				std::cout << "[Riot API] fetchInfo: PUUID " << pPlayer->getPUUID() << " not found in match participants." << std::endl;
				if (next) next(false);
				return;
			}

			pPlayer->setMatchInfo(matchInfo);
			if (next) next(true);
		} catch (const json::exception& e) {
			logMalformedResponse("fetchInfo", infoURL, e.what(), http.body);
			if (next) next(false);
		}
	});
};

void Riot::setName(std::shared_ptr<Player> pPlayer) {
	std::string nameURL = "https://americas.api.riotgames.com/riot/account/v1/accounts/by-puuid/" + pPlayer->getPUUID() + "?api_key=" + apiKey;
	botCluster.request(nameURL, dpp::m_get, [pPlayer, nameURL](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			logHttpError("setName", nameURL, http);
			return;
		}

		json nameJson;
		try {
			nameJson = json::parse(http.body);
		} catch (const json::parse_error& e) {
			logMalformedResponse("setName", nameURL, e.what(), http.body);
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
			logHttpError("fetchPUUID", idURL, http);
			if (next) next("");
			return;
		}

		try {
			json idJson = json::parse(http.body);
			std::string puuid = idJson.at("puuid").get<std::string>();
			if (next) next(puuid);
		} catch (const json::exception& e) {
			logMalformedResponse("fetchPUUID", idURL, e.what(), http.body);
			if (next) next("");
		}
	});
}

void Riot::fetchSummonerID(std::shared_ptr<Player> pPlayer) {
	std::string summonerURL = "https://na1.api.riotgames.com/tft/summoner/v1/summoners/by-puuid/" + pPlayer->getPUUID() + "?api_key=" + apiKey;
	botCluster.request(summonerURL, dpp::m_get, [pPlayer, summonerURL](const dpp::http_request_completion_t& http) {
		if (http.status != 200) {
			logHttpError("fetchSummonerID", summonerURL, http);
			return;
		}

		json summonerJson;
		try {
			summonerJson = json::parse(http.body);
		} catch (const json::parse_error& e) {
			logMalformedResponse("fetchSummonerID", summonerURL, e.what(), http.body);
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
			logHttpError("fetchLeague", leagueURL, http);
			if (next) next(false);
			return;
		}

		try {
			json leagueJson = json::parse(http.body);

			if (!leagueJson.is_array() || leagueJson.empty()) {
				std::cout << "[Riot API] fetchLeague: no league data for PUUID " << pPlayer->getPUUID() << std::endl;
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
			logMalformedResponse("fetchLeague", leagueURL, e.what(), http.body);
			if (next) next(false);
		}
	});
}

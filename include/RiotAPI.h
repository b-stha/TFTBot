/*
Class Riot handles Riot API interactions.
Fetches information using DPP's built in HTTP client asynchronously and updates Player objects.
*/

#ifndef RIOTAPI_H
#define RIOTAPI_H

#include <string>
#include <functional>
#include <dpp/dpp.h>
#include <memory>

class Player;

class Riot {
public:
    Riot(dpp::cluster& bot, const std::string& apiKey);
    void fetchMatchID(std::shared_ptr<Player> pPlayer, std::function<void(bool)> next = {});
    void fetchInfo(std::shared_ptr<Player> pPlayer, std::function<void(bool)> next = {});
    void setName(std::shared_ptr<Player> pPlayer);
    void fetchPUUID(const std::string& name, const std::string& tag, std::function<void(const std::string&)> next = {});
    void fetchSummonerID(std::shared_ptr<Player> pPlayer);
    void fetchLeague(std::shared_ptr<Player> pPlayer, std::function<void(bool)> next = {});

private:
    dpp::cluster& botCluster; // Reference to the bot cluster to make API calls.
    std::string apiKey;
};

#endif


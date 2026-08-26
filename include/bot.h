/* 
Highest level object that starts the Discord bot and smaller objects.
Stores all of the loaded game data, map of added users, Riot API handler object, and Worker object.
Contains some helper functions for formatting data into Discord embeds.
*/

#ifndef BOT_H
#define BOT_H

#include <atomic>
#include <dpp/dpp.h>
#include <memory>
#include "RiotAPI.h"
#include <unordered_map>
#include <mutex>

class Data;
class Player;
struct Worker;

class Bot {
public:
    Bot(const std::string& botToken, const std::string& riotApiKey);
    dpp::cluster& getBotCluster() { return botCluster; }
    Riot& getRiotObj() { return riotAPI; }
    std::vector<std::shared_ptr<Player>> getUserSnapshot(); // Returns a snapshot of the current users to avoid locking.
    void unitListStr(const Player& player, dpp::embed& embedObj, const Data& data);
    void traitListStr(const Player& player, dpp::embed& embedObj, const Data& data);
    std::string augListStr(const Player& player, const Data& data);
    void createRankedEmbed(const Player& player, const Data& data);
    void createDoubleUpEmbed(const Player& player, const Data& data);
    void createUnrankedEmbed(const Player& player, const Data& data);
    dpp::embed createPromoMsg(const Player& player, const Data& data, std::string queueType);
    std::shared_ptr<Data> getLoadedData() const { return pLoadedData; }
    Worker* getWorker() { return pWorker.get(); }
    void run();
private:
    void registerCommands();
    void readyHandler();
    dpp::cluster botCluster;
    Riot riotAPI;
    std::unordered_map<std::string, std::shared_ptr<Player>> userMap;
    std::unique_ptr<Worker> pWorker;
    std::shared_ptr<Data> pLoadedData;
    std::atomic<bool> isReady;
    std::mutex userMapMutex;
};

#endif

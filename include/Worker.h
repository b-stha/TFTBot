/*
Object to handle queueing and processing of added players as asynchronously as possible.
Conducts all post-initialization API calls and data processing and prevents main thread from being blocked.
*/

#ifndef WORKER_H
#define WORKER_H

#include <queue>
#include <memory>
#include <mutex>
#include <unordered_set>

class Player;
class Riot;
class Data;
class Bot;

class Worker {
public:
    void startTask();
    void finishTask();
    std::shared_ptr<Data> getData() const;
    bool enqueue(const std::shared_ptr<Player>& player);
    Worker(Bot* bot)
        : pMittens(bot) {}
private:
    std::unordered_set<std::string> queuedOrRunningPuuids; // tracks puuids that are either queued or currently being processed to prevent duplicates
    std::string activePuuid; 
	bool isRunning = false;
    std::queue<std::shared_ptr<Player>> playerQueue; // queue of players waiting to be processed
    std::mutex queueMutex;
    Bot* pMittens;
};

#endif
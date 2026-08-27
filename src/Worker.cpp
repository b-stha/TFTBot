#include "Worker.h"
#include "Player.h"
#include "RiotAPI.h"
#include "data.h"
#include "bot.h"

void Worker::startTask() {
    std::shared_ptr<Player> currPlayer = nullptr;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (isRunning || playerQueue.empty()) {
            return;
        }

        currPlayer = playerQueue.front();
        if (!currPlayer || currPlayer->getPUUID().empty()) {
            playerQueue.pop();
            return;
        }

        playerQueue.pop();
        isRunning = true;
        activePuuid = currPlayer->getPUUID();
    }

    auto doneOnce = std::make_shared<std::atomic_bool>(false);
    auto finishSafely = [this, doneOnce]() {
        bool expected = false;
        if (!doneOnce->compare_exchange_strong(expected, true)) {
            return;
        }
        this->finishTask();
    };

    pMittens->getRiotObj().fetchMatchID(currPlayer, [this, currPlayer, finishSafely](bool matchFetched) mutable {
        if (!matchFetched) {
            currPlayer->setPendingMatchID("");
            finishSafely();
            return;
        }
        pMittens->getRiotObj().fetchLeague(currPlayer, [this, currPlayer, finishSafely](bool leagueFetched) mutable {
            if (!leagueFetched) {
                currPlayer->setPendingMatchID("");
                finishSafely();
                return;
            }
            pMittens->getRiotObj().fetchInfo(currPlayer, [this, currPlayer, finishSafely](bool infoFetched) mutable {
                if (!infoFetched) {
                    currPlayer->setPendingMatchID("");
                    finishSafely();
                    return;
                }
                auto data = this->getData();
                int currQueueID = currPlayer->getMatchInfo().queueID;
                std::unordered_set<int> addedQueues = currPlayer->getAddedQueues();
                if (addedQueues.contains(currQueueID)) {
                    switch (currQueueID) {
                        case 1100: // ranked
                            pMittens->createRankedEmbed(*currPlayer, *data);
                            break;
                        case 1160: // double up
                            pMittens->createDoubleUpEmbed(*currPlayer, *data);
                            break;
                        default: // unranked
                            pMittens->createUnrankedEmbed(*currPlayer, *data);
                            break;
                    }
                }
                finishSafely();
            });
        });
    });
}

void Worker::finishTask() {
    bool shouldStartAnother = false;
    {
        std::lock_guard<std::mutex> lock(queueMutex);
        if (!activePuuid.empty()) {
            queuedOrRunningPuuids.erase(activePuuid);
            activePuuid.clear();
        }
        isRunning = false;
        shouldStartAnother = !playerQueue.empty();
    }

    if (shouldStartAnother) {
        startTask();
    }
}

std::shared_ptr<Data> Worker::getData() const {
    return pMittens->getLoadedData();
}

bool Worker::enqueue(const std::shared_ptr<Player>& player) {
    if (!player) return false;

    const std::string puuid = player->getPUUID();
    if (puuid.empty()) {
        return false;
    }

    std::lock_guard<std::mutex> lock(queueMutex);
    if (queuedOrRunningPuuids.contains(puuid)) {
        return false;
    }

    queuedOrRunningPuuids.insert(puuid);
    playerQueue.push(player);
    return true;
}

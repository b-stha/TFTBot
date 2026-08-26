#include "Player.h"
#include "RiotAPI.h"
#include "bot.h"
#include "Worker.h"
#include "data.h"
#include <dpp/dpp.h>
#include <atomic>
#include <memory>
#include <cstdlib>

std::atomic <bool> running = false;

namespace {
std::string readEnvOrEmpty(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? value : "";
}
}

void stop() {
    running = false;
}

int main() {
    const std::string botToken = readEnvOrEmpty("BOT_TOKEN");
    const std::string riotApiKey = readEnvOrEmpty("TFT_APIKEY");

    if (botToken.empty()) {
        std::cerr << "Missing required environment variable: BOT_TOKEN" << std::endl;
    }

    if (riotApiKey.empty()) {
        std::cerr << "Missing required environment variable: TFT_APIKEY" << std::endl;
    }

    if (botToken.empty() || riotApiKey.empty()) {
        return 1;
    }

    Bot mittens(botToken, riotApiKey);
    mittens.run();

    signal(SIGINT, [](int code) {
        running = false;
        });

    running = true;
    auto &bot = mittens.getBotCluster();
    Worker* worker = mittens.getWorker();
    try {
        while (running) {
            auto userSnapshot = mittens.getUserSnapshot();
            if (!userSnapshot.empty()) {
                for (auto& user : userSnapshot) {
                    if (worker->enqueue(user)) {
                        std::cout << "enqueued: " << user->getPUUID() << "\n";
                    } else {
                        std::cout << "skipped dup: " << user->getPUUID() << "\n";
                    }
                }
                worker->startTask();
            }
            std::this_thread::sleep_for(std::chrono::seconds(10));
        };
    }
    catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        
        return 1;
    }

    return 0;
}

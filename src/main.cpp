#include "Player.h"
#include "RiotAPI.h"
#include "bot.h"
#include "Worker.h"
#include "data.h"
#include <dpp/dpp.h>
#include <atomic>
#include <chrono>
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <memory>
#include <thread>

std::atomic<bool> running = false;

namespace {
std::string readEnvOrEmpty(const char* name) {
    const char* value = std::getenv(name);
    return value != nullptr ? value : "";
}
}

void stop(int) {
    running.store(false);
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

    std::signal(SIGINT, stop);
    std::signal(SIGTERM, stop);
    running.store(true);

    Bot mittens(botToken, riotApiKey);
    Worker* worker = mittens.getWorker();

    try {
        mittens.run();
        while (running.load()) {
            if (mittens.dataInitializationFailed()) {
                std::cerr << "Data initialization failed; shutting down." << std::endl;
                running.store(false);
                break;
            }

            if (mittens.getLoadedData()) {
                auto userSnapshot = mittens.getUserSnapshot();
                for (auto& user : userSnapshot) {
                    if (!running.load()) {
                        break;
                    }
                    if (worker->enqueue(user)) {
                        std::cout << "enqueued: " << user->getPUUID() << "\n";
                    }
                }
                worker->startTask();
            }

            for (int i = 0; i < 100 && running.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        running.store(false);
    }

    mittens.shutdown();
    return 0;
}

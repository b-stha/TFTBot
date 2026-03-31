#include "bot.h"
#include "RiotAPI.h"
#include "apikeys.h"
#include "data.h"
#include "Player.h"
#include "helpers.h"
#include "Worker.h"

Bot::Bot()
		: botCluster(BOT_TOKEN, dpp::i_default_intents | dpp::i_message_content), riotAPI(botCluster, TFT_APIKEY), pWorker(std::make_unique<Worker>(this)){
	;
	botCluster.on_log(dpp::utility::cout_logger());
	readyHandler();
	registerCommands();
}	

void Bot::run() {
	botCluster.start(dpp::st_return);
}

void Bot::registerCommands() {
    botCluster.on_slashcommand([this](const dpp::slashcommand_t& event) {
		const auto commandName = event.command.get_command_name();
        if (commandName == "ping") {
            event.reply("Pong!");
			return;
        }
		if (commandName == "add") {
			if (!isReady.load() || !pLoadedData) {
				event.reply("Bot is still initializing data, please try again later.");
				return;
			}
			dpp::command_interaction cmd_data = std::get<dpp::command_interaction>(event.command.data);
			dpp::snowflake currChannel = event.command.channel_id;
			std::string name = std::get<std::string>(cmd_data.options[0].value);
			std::cout << name << std::endl;
			std::vector<std::string> userInputArr;

			if (name.find("#") != std::string::npos) { 
				userInputArr = split(name, '#');
			}
			else {
				userInputArr.emplace_back(name);
				userInputArr.emplace_back("NA1");
			}

			const std::string queueOpt = std::get<std::string>(cmd_data.options[1].value);

			auto eventCopy = event;
			auto data = this->getLoadedData();
			riotAPI.fetchPUUID(userInputArr[0], userInputArr[1], [this, name, currChannel, userInputArr, eventCopy, queueOpt, data](const std::string& puuid) {
				bool isNewUser = false;
				std::shared_ptr<Player> pPlayer;
				{
					std::lock_guard<std::mutex> lock(this->userMapMutex);
					auto it = this->userMap.find(puuid);
					if (it == this->userMap.end()) {
						pPlayer = std::make_shared<Player>(puuid);
						pPlayer->setChannelID(currChannel);
						pPlayer->setNameTag(userInputArr[0], userInputArr[1]);
						this->userMap.emplace(puuid, pPlayer);
						isNewUser = true;
					}
					else {
						pPlayer = it->second;
					}
				}
				bool addedAnything = false;
				if (queueOpt == "all") {

					for (const std::string& opt : {"ranked", "double_up", "normal"}) {
						const int queueId = data->getQueueID(opt);
						if (pPlayer->getAddedQueues().count(queueId) == 0) {
							pPlayer->addQueue(queueId);
							addedAnything = true;
						}
					}

					if (addedAnything) {
						eventCopy.reply(name + " updated with all queues.");
					} else {
						eventCopy.reply(name + " already has all queues.");
					}
				}
				else {
					const int queueId = data->getQueueID(queueOpt);

					if (pPlayer->getAddedQueues().count(queueId) == 0) {
						pPlayer->addQueue(queueId);
						addedAnything = true;
					}
				}

				if (!isNewUser) {
					if (addedAnything) {
						pPlayer->setCurrMatch("");
						eventCopy.reply(name + " updated (" + queueOpt + ").");
					} else {
						eventCopy.reply(name + " already had " + queueOpt + ".");
					}
					return;
				}
				riotAPI.fetchLeague(pPlayer, [eventCopy, name, queueOpt](bool ok) mutable {
					if (!ok) {
						eventCopy.reply("Failed to fetch league info for " + name + ".");
						return;
					}
					eventCopy.reply(name + " successfully added (" + queueOpt + ").");
				});
			});
		}
	});
}

void Bot::readyHandler() {
    botCluster.on_ready([this](const dpp::ready_t& event) {
		std::thread([this]() {
			auto fData = Data::asyncCreateData(botCluster);
			try {
				auto readyData = fData.get();
				this->pLoadedData = std::move(readyData);
				this->isReady.store(true);
				std::cout << "Data initialization succeeded." << std::endl;
			} catch (const std::exception& e) {
				std::cerr << "Data initialization failed: " << e.what() << std::endl;
			}

		}).detach();

        if (dpp::run_once<struct register_bot_commands>()) {
            botCluster.global_command_create(dpp::slashcommand("ping", "Ping pong!", botCluster.me.id));

            dpp::slashcommand add;
            add.set_name("add");
            add.set_description("Format as name#tagline.");
            add.add_option(
                dpp::command_option(dpp::co_string, "username", "name#tag", true)
            );
			add.add_option(
				dpp::command_option(dpp::co_string, "queue", "Queue to add", true)
				.add_choice(dpp::command_option_choice("ranked", "ranked"))
				.add_choice(dpp::command_option_choice("normal", "normal"))
				.add_choice(dpp::command_option_choice("double up", "double_up"))
				.add_choice(dpp::command_option_choice("all", "all"))
			);

            botCluster.global_command_create(add, [this](const dpp::confirmation_callback_t& callback) {
                if (callback.is_error()) {
                    std::cout << callback.http_info.body << "\n";
                }
			});
		};
	});
};

/*
std::string augListStr(const Player& player) {
	std::string augListOutput = "";

	if (player.myMatchInfo.augments.empty()) {
		return "None";
	}

	for (const auto& augment : player.myMatchInfo.augments) {
        if (augmentData.count(augment) > 0) {
		    augListOutput += augmentData.at(augment)[1] + " " + augmentData.at(augment)[0] + "\n";
                continue;
            }
            augListOutput += "<:steamhappy:1123798178030964848> Placeholder Augment \n";
	}
	return augListOutput;
};*/

void Bot::unitListStr(const Player& player, dpp::embed& embedObj, const Data& data) {
	for (const auto& unit : player.getMatchInfo().units) {
		std::string apiName = unit.characterID, unitName, unitIconName;
		std::unordered_map<std::string, UnitInfo> unitData = data.getUnitData();
		const auto it = unitData.find(apiName);
		if (it != unitData.end()) {
			unitName = it->second.name;
			unitIconName = data.getEmote(lowerCase(apiName)) + " " + unitName;
		} else {
			unitName = "null";
		}

		unitName = setStrWidth(unitName, 10);
		std::string unitItems = "";
		std::string emptyItem = "<:transparent:1250910469330567292>";
		if (unit.items.empty()) {
			unitItems += (emptyItem * 3);
		}
		else {
			for (const auto& item : unit.items) {
				unitItems += data.getEmote(lowerCase(item)) + " "; 
			}
		}

		embedObj.add_field(
			"", 
			starCount(unit.tier) + "\n" + 
			unitIconName + "\n" +
			unitItems + "\n",
			true);
	}
};

void Bot::traitListStr(const Player& player, dpp::embed& embedObj, const Data& data) {
	for (const auto& trait : player.getMatchInfo().traits) {
		if (trait.style != 0) {
			std::string traitName, traitIcon;
			int currBreakpoint;
			const auto& traitData = data.getTraitData();
			const auto it = traitData.find(trait.apiName);
			if (it != traitData.end()) {
				traitName = it->second.name;
				std::string emojiName = lowerCase(trait.apiName) + "_" + std::to_string(trait.style);
				traitIcon = data.getEmote(emojiName);
				int traitIdx = trait.level - 1;
				const auto& breakpoints = it->second.breakpoints;
				if (traitIdx >= 0) {
					if (static_cast<size_t>(traitIdx) < breakpoints.size()) {
					currBreakpoint = breakpoints[traitIdx];
					} else {
						currBreakpoint = 0;
					}
				}
			} else {
				traitName = "null";
				currBreakpoint = 0;
			}

			traitName = setStrWidth(traitName, 10);
			embedObj.add_field(
				"", 
				traitIcon + " " + std::to_string(trait.numUnits) + "/" + std::to_string(currBreakpoint) + " " + traitName,
				true);
		}
	}
}

void Bot::createRankedEmbed(const Player& player, const Data& data) {
	std::string name = player.getFullName()[0];
	std::string profileURL = "https://tactics.tools/player/na/" + fillSpaces(name) + "/" + player.getFullName()[1] + "/";
	std::string matchResultURL = profileURL + player.getCurrMatchID();
	//std::string augmentList = augListStr(player);
	std::string playerTier = player.getRank().first;
	dpp::embed outEmbed = dpp::embed()
		.set_color(data.getRankColor().at(playerTier))
		.set_title(data.getPlacementData().at(player.getMatchInfo().placement) + " PLACE")
		.set_url(matchResultURL)
		.set_author(name + "'s ranked match result", profileURL, "")
		.set_thumbnail(data.getTacticianIcon(player.getMatchInfo().tacticianID))
		.add_field(
			getRankField(player, data, "RANKED"),
			"\n"
			"Duration: " + std::to_string(player.getTime()[0]) + ":" + std::to_string(player.getTime()[1]) + "\n"
			"Level: " + std::to_string(player.getMatchInfo().level) + "\n"
			"Gold Left: " + std::to_string(player.getMatchInfo().goldLeft) + "\n"
			"Board Value: " + std::to_string(player.getMatchInfo().boardValue),
			false
		)/*
		.add_field(
			"Augments",
			augmentList,
			false
		)*/
		.add_field(
			"Traits",
			"",
			false
		);
	traitListStr(player, outEmbed, data);
	outEmbed
		.add_field(
			"Units",
			"",
			false
		);
	unitListStr(player, outEmbed, data);

	dpp::message msg(player.getChannelID(), outEmbed);
	botCluster.message_create(msg);

	if (player.getRank().first != player.getPrevRanked()) {
		dpp::embed promoMsg = createPromoMsg(player, data, "RANKED");
		dpp::message promoMsgObj(player.getChannelID(), promoMsg);
		botCluster.message_create(promoMsgObj);
	}
}

void Bot::createDoubleUpEmbed(const Player& player, const Data& data) {
	std::string name = player.getFullName()[0];
	std::string profileURL = "https://tactics.tools/player/na/" + fillSpaces(name) + "/" + player.getFullName()[1] + "/";
	std::string matchResultURL = profileURL + player.getCurrMatchID();
	//std::string augmentList = augListStr(player);
	std::string playerTier = player.getDoubleUpRank().first;
	int doubleUpPlacement = (player.getMatchInfo().placement + 1) / 2;
	dpp::embed outEmbed = dpp::embed()
		.set_color(data.getRankColor().at(playerTier))
		.set_title(data.getPlacementData().at(doubleUpPlacement) + " PLACE (DOUBLE UP)")
		.set_url(matchResultURL)
		.set_author(name + "'s Double Up match result", profileURL, "")
		.set_thumbnail(data.getTacticianIcon(player.getMatchInfo().tacticianID))
		.add_field(
			getRankField(player, data, "DOUBLE_UP"),
			"\n"
			"Duration: " + std::to_string(player.getTime()[0]) + ":" + std::to_string(player.getTime()[1]) + "\n"
			"Level: " + std::to_string(player.getMatchInfo().level) + "\n"
			"Gold Left: " + std::to_string(player.getMatchInfo().goldLeft) + "\n"
			"Board Value: " + std::to_string(player.getMatchInfo().boardValue),
			false
		)/*
		.add_field(
			"Augments",
			augmentList,
			false
		)*/
		.add_field(
			"Traits",
			"",
			false
		);
	traitListStr(player, outEmbed, data);
	outEmbed
		.add_field(
			"Units",
			"",
			false
		);
	unitListStr(player, outEmbed, data);

	dpp::message msg(player.getChannelID(), outEmbed);
	botCluster.message_create(msg);

	if (player.getDoubleUpRank().first != player.getPrevDoubleUp()) {
		dpp::embed promoMsg = createPromoMsg(player, data, "DOUBLE_UP");
		dpp::message promoMsgObj(player.getChannelID(), promoMsg);
		botCluster.message_create(promoMsgObj);
	}
}

void Bot::createUnrankedEmbed(const Player& player, const Data& data) {
	std::string name = player.getFullName()[0];
	std::string profileURL = "https://tactics.tools/player/na/" + fillSpaces(name) + "/" + player.getFullName()[1] + "/";
	std::string matchResultURL = profileURL + player.getCurrMatchID();
	//std::string augmentList = augListStr(player);
	dpp::embed outEmbed = dpp::embed()
		.set_color(dpp::colors::gray)
		.set_title(data.getPlacementData().at(player.getMatchInfo().placement) + " PLACE")
		.set_url(matchResultURL)
		.set_author(name + "'s normal match result", profileURL, "")
		.set_thumbnail(data.getTacticianIcon(player.getMatchInfo().tacticianID))
		.add_field(
			"NORMAL",
			"\n"
			"Duration: " + std::to_string(player.getTime()[0]) + ":" + std::to_string(player.getTime()[1]) + "\n"
			"Level: " + std::to_string(player.getMatchInfo().level) + "\n"
			"Gold Left: " + std::to_string(player.getMatchInfo().goldLeft) + "\n"
			"Board Value: " + std::to_string(player.getMatchInfo().boardValue),
			false
		)/*
		.add_field(
			"Augments",
			augmentList,
			false
		)*/
		.add_field(
			"Traits",
			"",
			false
		);
	traitListStr(player, outEmbed, data);
	outEmbed
		.add_field(
			"Units",
			"",
			false
		);
	unitListStr(player, outEmbed, data);
}

dpp::embed Bot::createPromoMsg(const Player& player, const Data& data, std::string queueType) {
	std::string playerTier = "";
	if (queueType == "DOUBLE_UP") {
		playerTier = player.getDoubleUpRank().first;
	}
	else if (queueType == "RANKED") {
		playerTier = player.getRank().first;
	}
	std::string name = player.getFullName()[0];
	dpp::embed promoEmbed = dpp::embed()
		.set_color(data.getRankColor().at(playerTier))
		.set_title("PROMOTION")
		.set_thumbnail(data.getRankData().at(playerTier))
		.add_field(
			name + " has promoted to " + playerTier,
			"",
			false
		);

	return promoEmbed;
};

std::vector<std::shared_ptr<Player>> Bot::getUserSnapshot() {
	std::vector<std::shared_ptr<Player>> snapshot;
	std::lock_guard<std::mutex> lock(this->userMapMutex);
	for (const auto& [puuid, playerPtr] : this->userMap) {
		snapshot.emplace_back(playerPtr);
	}
	return snapshot;
}

/*
Object to hold temporary match information for each player.
Updated after each new match and used to create Discord embeds.
Includes Unit and Trait structs to hold relevant info for future embedding.
from_json functions to convert from JSON to C++ objects using nlohmann/json library.
*/

#ifndef MATCHINFO_H
#define MATCHINFO_H

#include <string>
#include <vector>
#include <nlohmann/json.hpp>
#include "helpers.h"

struct Unit {
	std::string characterID;
	std::vector<std::string> items;
	int rarity = 0;
	int tier = 0;
};
inline void from_json(const nlohmann::json& j, Unit& u)
{
    j.at("character_id").get_to(u.characterID);
	u.characterID = lowerCase(u.characterID);
    j.at("itemNames").get_to(u.items);
    j.at("rarity").get_to(u.rarity);
    j.at("tier").get_to(u.tier);
};

struct Trait {
	std::string apiName;
	int level;
	int style;
	int numUnits;
};
inline void from_json(const nlohmann::json& j, Trait& t) {
    j.at("name").get_to(t.apiName);
    j.at("style").get_to(t.style);
    j.at("tier_current").get_to(t.level);
    j.at("num_units").get_to(t.numUnits);
};

struct MatchInfo {
	//std::vector<std::string> augments;
	std::vector<Trait> traits;
	std::vector<Unit> units;
	int gameLenMin = 0;
	int gameLenSec = 0;
	int goldLeft = 0;
	int level = 0;
	int placement = 0;
	int boardValue;
	int tacticianID;
	int queueID;
	int calcBoardValue();
};
inline void from_json(const nlohmann::json& j, MatchInfo& p)
{
    //j.at("augments").get_to(p.augments);
    j.at("units").get_to(p.units);
    std::reverse(p.units.begin(), p.units.end());
    j.at("gold_left").get_to(p.goldLeft);
    j.at("level").get_to(p.level);
    j.at("placement").get_to(p.placement);
    j.at("traits").get_to(p.traits);
    j.at("companion").at("item_ID").get_to(p.tacticianID);
    std::sort(p.traits.begin(), p.traits.end(), 
        [](const Trait& t1, const Trait& t2) {
	    return t1.style > t2.style;
    });
};

#endif
/*
helper functions for various unclassified tasks such as string manipulation, formatting, etc.
*/

#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <vector>
#include <memory>

class Player;
class Data;

std::string setStrWidth(const std::string& str, int len); // fixes string width to len by padding or truncating
std::string operator * (std::string a, unsigned int b); // overload * operator to repeat strings
std::string starCount(const int& tier); // converts tier number to string of stars
std::vector<std::string> split(const std::string& s, char delim); // splits string s using delim
bool notPlayerExists(const std::vector<std::shared_ptr<Player>>& players, const std::string& puuid);
std::string fillSpaces(std::string input); // fills spaces in url parameters with "%20"
std::string getRankField(const Player& player, const Data& data, std::string queueType); // gets formatted rank field from Data for a given player and queue type
std::string lowerCase(std::string str);

#endif

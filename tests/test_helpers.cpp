#include "helpers.h"
#include "MatchInfo.h"

#include <cassert>
#include <iostream>
#include <string>
#include <vector>

int main() {
    std::string padded = setStrWidth("abc", 5);
    assert(padded == "abc  ");

    std::string trimmed = setStrWidth("abcdef", 4);
    assert(trimmed == "abcd");

    std::vector<std::string> parts = split("alpha#beta#gamma", '#');
    assert(parts.size() == 3);
    assert(parts[0] == "alpha");
    assert(parts[1] == "beta");
    assert(parts[2] == "gamma");

    assert(fillSpaces("hello world") == "hello%20world");
    assert(lowerCase("ABC123") == "abc123");
    assert(starCount(3) == ":star::star::star:");

    MatchInfo matchInfo;
    Unit unitA;
    unitA.characterID = "unit_a";
    unitA.rarity = 2;
    unitA.tier = 2;

    Unit unitB;
    unitB.characterID = "unit_b";
    unitB.rarity = 1;
    unitB.tier = 1;

    matchInfo.units.push_back(unitA);
    matchInfo.units.push_back(unitB);
    assert(matchInfo.calcBoardValue() == 11);

    std::cout << "All tests passed." << std::endl;
    return 0;
}

#ifndef DA_INPUT_PARSER_H
#define DA_INPUT_PARSER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "da/types.h"

int parseProgramPoint(const std::string &token, bool &isDef, bool &isLastUse);

std::unordered_map<std::string, std::vector<LiveRange>> parseLiveRanges(const std::string &filename);

AllocConfig parseConfig(const std::string &filename);

#endif // DA_INPUT_PARSER_H

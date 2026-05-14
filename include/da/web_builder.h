#ifndef DA_WEB_BUILDER_H
#define DA_WEB_BUILDER_H

#include <string>
#include <unordered_map>
#include <vector>

#include "da/types.h"

std::vector<Web> buildWebsForVariable(const std::vector<LiveRange> &ranges, int &startId);

std::vector<Web> buildAllWebs(const std::unordered_map<std::string, std::vector<LiveRange>> &allRanges);

#endif // DA_WEB_BUILDER_H

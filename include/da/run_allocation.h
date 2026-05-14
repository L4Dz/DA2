#ifndef DA_RUN_ALLOCATION_H
#define DA_RUN_ALLOCATION_H

#include <string>
#include <vector>

#include "da/types.h"

std::vector<Web> runAllocation(const std::string &rangesFile,
                               const std::string &configFile,
                               const std::string &outputFile);

#endif // DA_RUN_ALLOCATION_H

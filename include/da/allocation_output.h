#ifndef DA_ALLOCATION_OUTPUT_H
#define DA_ALLOCATION_OUTPUT_H

#include <string>
#include <vector>

#include "da/types.h"
#include "graph/graph_bundle.hpp"

std::string formatWebPoints(const Web &w);

void writeOutput(const std::vector<Web> &webs, const Graph<int> &G, const std::string &outFile);

#endif // DA_ALLOCATION_OUTPUT_H

#ifndef DA_INTERFERENCE_GRAPH_BUILDER_H
#define DA_INTERFERENCE_GRAPH_BUILDER_H

#include <vector>

#include "da/types.h"
#include "graph/graph_bundle.hpp"

Graph<int> buildInterferenceGraph(const std::vector<Web> &webs);

std::vector<Web> collectWebsFromGraph(const Graph<int> &G);

#endif // DA_INTERFERENCE_GRAPH_BUILDER_H

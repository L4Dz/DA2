#include "da/run_allocation.h"

#include <iostream>
#include <stdexcept>

#include "da/allocation_output.h"
#include "da/input_parser.h"
#include "da/interference_graph_builder.h"
#include "da/web_builder.h"

std::vector<Web> runAllocation(const std::string &rangesFile,
                               const std::string &configFile,
                               const std::string &outputFile) {
    auto liveRanges = parseLiveRanges(rangesFile);
    auto cfg        = parseConfig(configFile);

    std::vector<Web> webs = buildAllWebs(liveRanges);

    Graph<int> G = buildInterferenceGraph(webs);

    bool success = false;

    if (cfg.algorithm == "basic") {
        G.greedyColoring(cfg.numRegisters);
        success = G.getSpilledVertices().empty();

    } else if (cfg.algorithm == "spilling") {
        success = G.allocateWithSpilling(cfg.numRegisters, cfg.algorithmParam);

    } else if (cfg.algorithm == "splitting") {
        success = G.allocateWithSplitting(cfg.numRegisters, cfg.algorithmParam);

    } else if (cfg.algorithm == "free") {
        const int spillBudget = static_cast<int>(webs.size());
        G.greedyColoringDSatur(cfg.numRegisters);
        if (!G.getSpilledVertices().empty()) {
            for (auto *v : G.getVertexSet()) {
                v->setSpilled(false);
                v->setDisabled(false);
                v->setColor(-1);
            }
            G.greedyColoring(cfg.numRegisters);
        }
        if (!G.getSpilledVertices().empty())
            success = G.allocateWithSpilling(cfg.numRegisters, spillBudget);
        else
            success = true;

    } else {
        throw std::runtime_error("Unknown algorithm: " + cfg.algorithm);
    }

    if (!success)
        std::cerr << "WARNING: Could not colour the interference graph with "
                  << cfg.numRegisters << " registers within the given constraints.\n";

    std::vector<Web> outWebs = collectWebsFromGraph(G);

    writeOutput(outWebs, G, outputFile);

    return outWebs;
}

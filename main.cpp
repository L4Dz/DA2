// main.cpp
// Project 2 – Compiler Register Allocation – DA Spring 2026
//
// Usage (batch):   ./myProg -b ranges.txt registers.txt allocation.txt
// Usage (menu):    ./myProg

#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include "data_structures/registerallocator.h"

// ── helpers ───────────────────────────────────────────────────────────────────

static void printSeparator() {
    std::cout << "────────────────────────────────────────────────────\n";
}

static void printWebSummary(const std::vector<Web> &webs) {
    printSeparator();
    std::cout << "Webs (" << webs.size() << " total):\n";
    for (const auto &w : webs) {
        std::cout << "  web" << w.webId
                  << "  [" << w.varName << "]  "
                  << formatWebPoints(w);
        if      (w.reg == -2) std::cout << "  →  M (memory)\n";
        else if (w.reg >= 0)  std::cout << "  →  r" << w.reg << "\n";
        else                  std::cout << "  →  (unassigned)\n";
    }
    printSeparator();

    // Count registers used
    int regsUsed = 0;
    for (const auto &w : webs)
        if (w.reg >= 0 && w.reg + 1 > regsUsed) regsUsed = w.reg + 1;

    std::cout << "Registers used: " << regsUsed << "\n";
}

// ── batch mode ────────────────────────────────────────────────────────────────

static int batchMode(const std::string &rangesFile,
                     const std::string &configFile,
                     const std::string &outputFile) {
    try {
        auto webs = runAllocation(rangesFile, configFile, outputFile);
        printWebSummary(webs);
        std::cout << "Output written to: " << outputFile << "\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}

// ── interactive menu ──────────────────────────────────────────────────────────

static std::string promptString(const std::string &prompt) {
    std::string s;
    std::cout << prompt;
    std::getline(std::cin, s);
    return s;
}

static int promptInt(const std::string &prompt) {
    std::string s = promptString(prompt);
    try { return std::stoi(s); } catch (...) { return -1; }
}

static void menuMode() {
    std::string rangesFile, configFile, outputFile;
    std::vector<Web> webs;
    bool loaded = false;

    while (true) {
        printSeparator();
        std::cout << "  Register Allocation Tool – DA 2026\n";
        printSeparator();
        std::cout << "  1. Load input files\n";
        std::cout << "  2. Run allocation (basic)\n";
        std::cout << "  3. Run allocation (spilling)\n";
        std::cout << "  4. Run allocation (splitting)\n";
        std::cout << "  5. Run allocation (free)\n";
        std::cout << "  6. Show current web/register summary\n";
        std::cout << "  7. Write output to file\n";
        std::cout << "  0. Exit\n";
        printSeparator();

        int choice = promptInt("Choice: ");

        switch (choice) {
        // ── 1: Load files ──────────────────────────────────────────────────
        case 1: {
            rangesFile = promptString("Live-ranges file: ");
            configFile = promptString("Config file (registers/algorithm): ");
            outputFile = promptString("Output file: ");
            try {
                auto lr  = parseLiveRanges(rangesFile);
                auto cfg = parseConfig(configFile);
                webs = buildAllWebs(lr);
                std::cout << "Loaded " << webs.size() << " web(s) from "
                          << lr.size() << " variable(s).\n";
                std::cout << "Config: " << cfg.numRegisters << " register(s), "
                          << "algorithm = " << cfg.algorithm;
                if (cfg.algorithmParam > 0) std::cout << ", K = " << cfg.algorithmParam;
                std::cout << "\n";
                loaded = true;
            } catch (const std::exception &e) {
                std::cerr << "ERROR: " << e.what() << "\n";
            }
            break;
        }

        // ── 2-5: Run allocation ────────────────────────────────────────────
        case 2: case 3: case 4: case 5: {
            if (!loaded) { std::cerr << "Load files first (option 1).\n"; break; }

            AllocConfig cfg;
            try { cfg = parseConfig(configFile); }
            catch (const std::exception &e) { std::cerr << "ERROR: " << e.what() << "\n"; break; }

            // Override algorithm from menu choice
            if      (choice == 2) cfg.algorithm = "basic";
            else if (choice == 3) cfg.algorithm = "spilling";
            else if (choice == 4) cfg.algorithm = "splitting";
            else                  cfg.algorithm = "free";

            if (choice == 3 || choice == 4) {
                cfg.algorithmParam = promptInt("Max webs to spill/split (K): ");
                if (cfg.algorithmParam < 0) cfg.algorithmParam = 0;
            }

            try {
                // Rebuild graph for a fresh run
                auto lrMap = parseLiveRanges(rangesFile);
                webs = buildAllWebs(lrMap);
                Graph<int> G = buildInterferenceGraph(webs);

                bool success = false;
                if (cfg.algorithm == "basic") {
                    G.greedyColoring(cfg.numRegisters);
                    success = G.getSpilledVertices().empty();
                } else if (cfg.algorithm == "spilling") {
                    success = G.allocateWithSpilling(cfg.numRegisters, cfg.algorithmParam);
                } else if (cfg.algorithm == "splitting") {
                    success = G.allocateWithSplitting(cfg.numRegisters, cfg.algorithmParam);
                } else {
                    G.greedyColoring(cfg.numRegisters);
                    if (!G.getSpilledVertices().empty())
                        success = G.allocateWithSpilling(cfg.numRegisters,
                                                         static_cast<int>(webs.size()));
                    else success = true;
                }

                for (auto &w : webs) {
                    Vertex<int> *v = G.findVertex(w.webId);
                    if (v) w.reg = v->getColor();
                }

                if (!success)
                    std::cerr << "WARNING: Could not colour the graph with "
                              << cfg.numRegisters << " registers.\n";
                printWebSummary(webs);

            } catch (const std::exception &e) {
                std::cerr << "ERROR: " << e.what() << "\n";
            }
            break;
        }

        // ── 6: Show summary ────────────────────────────────────────────────
        case 6:
            if (!loaded) std::cerr << "Load files first (option 1).\n";
            else         printWebSummary(webs);
            break;

        // ── 7: Write output ────────────────────────────────────────────────
        case 7: {
            if (!loaded) { std::cerr << "Load files first (option 1).\n"; break; }
            if (outputFile.empty()) outputFile = promptString("Output file: ");
            try {
                Graph<int> G = buildInterferenceGraph(webs);
                writeOutput(webs, G, outputFile);
                std::cout << "Written to " << outputFile << "\n";
            } catch (const std::exception &e) {
                std::cerr << "ERROR: " << e.what() << "\n";
            }
            break;
        }

        // ── 0: Exit ────────────────────────────────────────────────────────
        case 0:
            std::cout << "Bye!\n";
            return;

        default:
            std::cerr << "Invalid option.\n";
        }
    }
}

// ── entry point ───────────────────────────────────────────────────────────────

int main(int argc, char *argv[]) {
    // Batch mode: ./myProg -b ranges.txt registers.txt allocation.txt
    if (argc == 5 && std::string(argv[1]) == "-b") {
        return batchMode(argv[2], argv[3], argv[4]);
    }

    if (argc != 1) {
        std::cerr << "Usage:\n"
                  << "  " << argv[0] << "                          (interactive menu)\n"
                  << "  " << argv[0] << " -b ranges.txt regs.txt out.txt  (batch)\n";
        return 1;
    }

    menuMode();
    return 0;
}


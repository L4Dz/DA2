// RegisterAllocator.h
// Handles: parsing, web construction, interference graph building, output
// Project 2 – Compiler Register Allocation – DA Spring 2026

#ifndef REGISTER_ALLOCATOR_H
#define REGISTER_ALLOCATOR_H

#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>
#include <stdexcept>
#include "Graph.h"

// =============================================================================
//  Data Structures
// =============================================================================

/**
 * @brief One live range for a variable.
 *
 *  A live range is a sorted list of program-point line numbers.
 *  The first point is a definition ('+') and the last is a use ('-').
 *  Intermediate points are "pass-through" lines where the value is live.
 */
struct LiveRange {
    std::string varName;          ///< owning variable
    std::vector<int> points;      ///< sorted line numbers (raw integers)
    std::vector<int> defPoints;   ///< subset of points that are definitions (+)
    std::vector<int> usePoints;   ///< subset of points that are last-uses   (-)
    bool defFirst = true;         ///< first point is a definition (+)
    bool useLast  = true;         ///< last  point is a use        (-)

    int start() const { return points.empty() ? -1 : points.front(); }
    int end()   const { return points.empty() ? -1 : points.back(); }

    /// Two live ranges overlap if their point-sets share at least one line number.
    bool overlapsWith(const LiveRange &other) const {
        for (int p : points)
            for (int q : other.points)
                if (p == q) return true;
        return false;
    }
};

/**
 * @brief A web = union of overlapping live ranges of the same variable.
 *
 *  Built by merging live ranges that share at least one program point.
 *  The merged point list is sorted and deduplicated.
 */
struct Web {
    int webId = -1;
    std::string varName;
    std::vector<int> points;           ///< sorted, deduplicated program points
    std::vector<int> defPoints;        ///< subset that are definitions (+)
    std::vector<int> usePoints;        ///< subset that are last-uses  (-)
    bool spilled  = false;
    int  reg      = -1;                ///< assigned register (-1 = none, -2 = memory)

    int start() const { return points.empty() ? -1 : points.front(); }
    int end()   const { return points.empty() ? -1 : points.back(); }

    /**
     * @brief Two webs interfere if their point-sets share a line number,
     *        UNLESS the only shared point has one web ending with a USE and
     *        the other beginning with a DEFINITION at that same line
     *        (in which case they do NOT interfere – see spec §2.5).
     */
    bool interferesWith(const Web &other) const {
        for (int p : points) {
            for (int q : other.points) {
                if (p != q) continue;

                // Shared point found. Check the non-interference exception:
                // this ends at p with a USE  AND  other starts at p with a DEF
                bool thisEndsUse    = (p == end()         &&
                                       std::find(usePoints.begin(), usePoints.end(), p) != usePoints.end());
                bool otherStartsDef = (p == other.start() &&
                                       std::find(other.defPoints.begin(), other.defPoints.end(), p) != other.defPoints.end());

                // Symmetric case
                bool otherEndsUse   = (p == other.end()   &&
                                       std::find(other.usePoints.begin(), other.usePoints.end(), p) != other.usePoints.end());
                bool thisStartsDef  = (p == start()        &&
                                       std::find(defPoints.begin(), defPoints.end(), p) != defPoints.end());

                if ((thisEndsUse && otherStartsDef) || (otherEndsUse && thisStartsDef))
                    continue; // this shared point does NOT cause interference

                return true;  // genuine interference
            }
        }
        return false;
    }
};

/**
 * @brief Configuration read from the registers/algorithm input file.
 */
struct AllocConfig {
    int numRegisters = 0;
    /// "basic" | "spilling" | "splitting" | "free"
    std::string algorithm = "basic";
    int algorithmParam = 0;  ///< K for spilling/splitting; 0 for basic/free
};

// =============================================================================
//  Parser
// =============================================================================

/**
 * @brief Parse a single program-point token such as "7+", "10-", "8".
 *
 *  Sets isDef=true if the token ends with '+', isLastUse=true if it ends with '-'.
 *  Returns the integer line number.
 *
 * @complexity O(|token|)
 */
inline int parseProgramPoint(const std::string &token, bool &isDef, bool &isLastUse) {
    isDef = isLastUse = false;
    if (token.empty()) throw std::invalid_argument("Empty program-point token");

    char last = token.back();
    std::string numStr = token;
    if (last == '+') { isDef = true;     numStr.pop_back(); }
    else if (last == '-') { isLastUse = true; numStr.pop_back(); }

    try {
        return std::stoi(numStr);
    } catch (...) {
        throw std::invalid_argument("Invalid program-point token: " + token);
    }
}

/**
 * @brief Parse the live-ranges input file (Figure 7 of the spec).
 *
 *  Format per data line:
 *    varName: pt1, pt2, ..., ptN
 *  where the first point ends with '+' and the last with '-'.
 *  Lines starting with '#' are comments.
 *
 * @param filename  Path to the live-ranges file.
 * @return          All live ranges, grouped by variable name.
 * @throws std::runtime_error on file or parse errors.
 * @complexity O(L · P) where L = lines, P = points per line
 */
inline std::unordered_map<std::string, std::vector<LiveRange>>
parseLiveRanges(const std::string &filename) {
    std::ifstream fin(filename);
    if (!fin.is_open())
        throw std::runtime_error("Cannot open live-ranges file: " + filename);

    std::unordered_map<std::string, std::vector<LiveRange>> result;
    std::string line;
    int lineNum = 0;

    while (std::getline(fin, line)) {
        ++lineNum;
        // Strip leading/trailing whitespace
        auto ltrim = [](std::string &s){ s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c){ return !std::isspace(c); })); };
        auto rtrim = [](std::string &s){ s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), s.end()); };
        ltrim(line); rtrim(line);

        if (line.empty() || line[0] == '#') continue;

        // Split at first ':'
        auto colonPos = line.find(':');
        if (colonPos == std::string::npos)
            throw std::runtime_error("Line " + std::to_string(lineNum) + ": missing ':'");

        std::string varName = line.substr(0, colonPos);
        ltrim(varName); rtrim(varName);
        if (varName.empty())
            throw std::runtime_error("Line " + std::to_string(lineNum) + ": empty variable name");

        std::string pointsStr = line.substr(colonPos + 1);

        LiveRange lr;
        lr.varName = varName;

        // Tokenise by ','
        std::istringstream iss(pointsStr);
        std::string token;
        bool firstToken = true;
        while (std::getline(iss, token, ',')) {
            ltrim(token); rtrim(token);
            if (token.empty()) continue;

            bool isDef = false, isLastUse = false;
            int pt = parseProgramPoint(token, isDef, isLastUse);
            lr.points.push_back(pt);

            if (isDef)     lr.defPoints.push_back(pt);
            if (isLastUse) lr.usePoints.push_back(pt);

            if (firstToken) { lr.defFirst = isDef;  firstToken = false; }
            lr.useLast = isLastUse; // updated at each token; final value is last token's
        }

        if (lr.points.empty())
            throw std::runtime_error("Line " + std::to_string(lineNum) + ": no program points");

        // Sort points
        // (preserve def/use metadata – they are subsets so just sort them too)
        std::sort(lr.points.begin(),    lr.points.end());
        std::sort(lr.defPoints.begin(), lr.defPoints.end());
        std::sort(lr.usePoints.begin(), lr.usePoints.end());

        result[varName].push_back(lr);
    }

    if (result.empty())
        throw std::runtime_error("Live-ranges file is empty or has no valid entries");

    return result;
}

/**
 * @brief Parse the registers/algorithm configuration file (Figure 9 of the spec).
 *
 *  Format:
 *    registers: N
 *    algorithm: basic | spilling, K | splitting, K | free
 *
 * @param filename  Path to the config file.
 * @return          Populated AllocConfig struct.
 * @throws std::runtime_error on file or parse errors.
 * @complexity O(lines)
 */
inline AllocConfig parseConfig(const std::string &filename) {
    std::ifstream fin(filename);
    if (!fin.is_open())
        throw std::runtime_error("Cannot open config file: " + filename);

    AllocConfig cfg;
    std::string line;
    int lineNum = 0;

    auto ltrim = [](std::string &s){ s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c){ return !std::isspace(c); })); };
    auto rtrim = [](std::string &s){ s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c){ return !std::isspace(c); }).base(), s.end()); };

    while (std::getline(fin, line)) {
        ++lineNum;
        ltrim(line); rtrim(line);
        if (line.empty() || line[0] == '#') continue;

        auto colonPos = line.find(':');
        if (colonPos == std::string::npos) continue;

        std::string key   = line.substr(0, colonPos);
        std::string value = line.substr(colonPos + 1);
        ltrim(key); rtrim(key);
        ltrim(value); rtrim(value);

        if (key == "registers") {
            try { cfg.numRegisters = std::stoi(value); }
            catch (...) { throw std::runtime_error("Invalid register count: " + value); }
            if (cfg.numRegisters < 1)
                throw std::runtime_error("Register count must be >= 1");

        } else if (key == "algorithm") {
            // Value may be "basic", "free", "spilling, 2", "splitting, 3"
            auto commaPos = value.find(',');
            if (commaPos == std::string::npos) {
                cfg.algorithm = value;
                ltrim(cfg.algorithm); rtrim(cfg.algorithm);
            } else {
                cfg.algorithm = value.substr(0, commaPos);
                ltrim(cfg.algorithm); rtrim(cfg.algorithm);
                std::string paramStr = value.substr(commaPos + 1);
                ltrim(paramStr); rtrim(paramStr);
                try { cfg.algorithmParam = std::stoi(paramStr); }
                catch (...) { throw std::runtime_error("Invalid algorithm parameter: " + paramStr); }
            }
        }
    }

    if (cfg.numRegisters == 0)
        throw std::runtime_error("Config file missing 'registers' entry");

    return cfg;
}

// =============================================================================
//  Web Construction
// =============================================================================

/**
 * @brief Build webs for one variable by union-find style merging.
 *
 *  Two live ranges of the same variable are merged into one web if their
 *  point-sets overlap (share at least one program point).  The result is a
 *  minimal set of webs such that no two webs of the same variable overlap.
 *
 * @param ranges   All live ranges for one variable (same varName).
 * @param startId  First web ID to assign.
 * @return         Vector of constructed webs.
 * @complexity O(R² · P) where R = ranges, P = points per range
 */
inline std::vector<Web> buildWebsForVariable(const std::vector<LiveRange> &ranges, int &startId) {
    // Start each range as its own candidate web.
    std::vector<Web> webs;
    for (const auto &lr : ranges) {
        Web w;
        w.varName   = lr.varName;
        w.points    = lr.points;
        w.defPoints = lr.defPoints;
        w.usePoints = lr.usePoints;
        webs.push_back(w);
    }

    // Repeatedly merge any two webs that overlap.
    bool merged = true;
    while (merged) {
        merged = false;
        for (std::size_t i = 0; i < webs.size() && !merged; ++i) {
            for (std::size_t j = i + 1; j < webs.size(); ++j) {
                // Check overlap
                bool overlap = false;
                for (int p : webs[i].points) {
                    if (std::find(webs[j].points.begin(), webs[j].points.end(), p)
                        != webs[j].points.end()) {
                        overlap = true; break;
                    }
                }
                if (!overlap) continue;

                // Merge j into i
                for (int p : webs[j].points)
                    if (std::find(webs[i].points.begin(), webs[i].points.end(), p) == webs[i].points.end())
                        webs[i].points.push_back(p);
                for (int p : webs[j].defPoints)
                    if (std::find(webs[i].defPoints.begin(), webs[i].defPoints.end(), p) == webs[i].defPoints.end())
                        webs[i].defPoints.push_back(p);
                for (int p : webs[j].usePoints)
                    if (std::find(webs[i].usePoints.begin(), webs[i].usePoints.end(), p) == webs[i].usePoints.end())
                        webs[i].usePoints.push_back(p);

                std::sort(webs[i].points.begin(),    webs[i].points.end());
                std::sort(webs[i].defPoints.begin(), webs[i].defPoints.end());
                std::sort(webs[i].usePoints.begin(), webs[i].usePoints.end());

                webs.erase(webs.begin() + j);
                merged = true;
                break;
            }
        }
    }

    // Assign IDs
    for (auto &w : webs) w.webId = startId++;
    return webs;
}

/**
 * @brief Build all webs from all variables' live ranges.
 *
 * @param allRanges  Output of parseLiveRanges().
 * @return           Flat vector of all webs, each with a unique webId.
 * @complexity O(V · R² · P)
 */
inline std::vector<Web> buildAllWebs(
    const std::unordered_map<std::string, std::vector<LiveRange>> &allRanges)
{
    std::vector<Web> allWebs;
    int nextId = 0;

    // Iterate in a deterministic order (sort by variable name)
    std::vector<std::string> varNames;
    for (const auto &kv : allRanges) varNames.push_back(kv.first);
    std::sort(varNames.begin(), varNames.end());

    for (const auto &name : varNames) {
        auto webs = buildWebsForVariable(allRanges.at(name), nextId);
        for (auto &w : webs) allWebs.push_back(w);
    }

    return allWebs;
}

// =============================================================================
//  Interference Graph Construction
// =============================================================================

/**
 * @brief Build an interference graph from a list of webs.
 *
 *  Each web becomes a vertex (info = webId).
 *  A bidirectional edge is added between two webs if they interfere
 *  (see Web::interferesWith()).
 *
 * @param webs  All webs (from buildAllWebs).
 * @return      Interference graph where vertex info = webId (int).
 * @complexity  O(W²) where W = number of webs
 */
inline Graph<int> buildInterferenceGraph(const std::vector<Web> &webs) {
    Graph<int> G;

    // Add one vertex per web
    for (const auto &w : webs) {
        G.addVertex(w.webId);
        Vertex<int> *v = G.findVertex(w.webId);
        v->setWebId(w.webId);
        v->setProgramPoints(w.points);
        v->addVarName(w.varName);
    }

    // Add interference edges
    for (std::size_t i = 0; i < webs.size(); ++i) {
        for (std::size_t j = i + 1; j < webs.size(); ++j) {
            if (webs[i].interferesWith(webs[j])) {
                G.addBidirectionalEdge(webs[i].webId, webs[j].webId, 1.0);
            }
        }
    }

    return G;
}

// =============================================================================
//  Output Writer
// =============================================================================

/**
 * @brief Format a web's program points as the spec requires.
 *
 *  First point gets '+', last point gets '-', others are plain integers.
 *  Example: "1+,2,3,4,5,6-"
 *
 * @complexity O(P)
 */
inline std::string formatWebPoints(const Web &w) {
    if (w.points.empty()) return "";
    std::ostringstream oss;
    for (std::size_t i = 0; i < w.points.size(); ++i) {
        if (i > 0) oss << ',';
        oss << w.points[i];
        if (i == 0)                        oss << '+';
        else if (i == w.points.size() - 1) oss << '-';
    }
    return oss.str();
}

/**
 * @brief Write the allocation result to a file (Figures 10/11 of the spec).
 *
 *  If all webs are spilled (registers used = 0), writes the infeasible format
 *  (Figure 11) and prints a warning to stderr.
 *
 * @param webs      All webs (with reg field populated after colouring).
 * @param G         The interference graph (for colour→register mapping).
 * @param outFile   Output file path.
 * @complexity      O(W · P)
 */
inline void writeOutput(const std::vector<Web> &webs,
                        const Graph<int>       &G,
                        const std::string      &outFile)
{
    // Count how many registers are actually used
    int regsUsed = 0;
    for (const auto &w : webs)
        if (w.reg >= 0 && w.reg + 1 > regsUsed)
            regsUsed = w.reg + 1;

    bool allSpilled = (regsUsed == 0);
    if (allSpilled)
        std::cerr << "WARNING: Register allocation was not possible with the given number of registers. "
                  << "All webs assigned to memory.\n";

    std::ofstream fout(outFile);
    if (!fout.is_open())
        throw std::runtime_error("Cannot open output file: " + outFile);

    // ── Web listing ──────────────────────────────────────────────────────────
    fout << "# Total number of webs followed by the listing of the program points of each one\n";
    fout << "# program points in each web are sorted in ascending order\n";
    fout << "webs: " << webs.size() << '\n';

    for (const auto &w : webs)
        fout << "web" << w.webId << ": " << formatWebPoints(w) << '\n';

    // ── Register assignment ───────────────────────────────────────────────────
    fout << "# Total number of registers used, followed by assignment to webs\n";
    fout << "registers: " << (allSpilled ? 0 : regsUsed) << '\n';

    if (allSpilled) {
        for (const auto &w : webs)
            fout << "M: web" << w.webId << '\n';
    } else {
        // Group webs by register
        std::unordered_map<int, std::vector<int>> regToWebs;
        for (const auto &w : webs) {
            if (w.reg >= 0)       regToWebs[w.reg].push_back(w.webId);
            else if (w.reg == -2) regToWebs[-2].push_back(w.webId);
        }

        // Print register assignments in register order
        for (int r = 0; r < regsUsed; ++r) {
            if (regToWebs.count(r))
                for (int wid : regToWebs[r])
                    fout << "r" << r << ": web" << wid << '\n';
        }
        // Print memory assignments
        if (regToWebs.count(-2))
            for (int wid : regToWebs[-2])
                fout << "M: web" << wid << '\n';
    }
}

// =============================================================================
//  Top-level orchestrator
// =============================================================================

/**
 * @brief Run the full register allocation pipeline.
 *
 *  1. Parse live ranges and config.
 *  2. Build webs.
 *  3. Build interference graph.
 *  4. Run the selected algorithm.
 *  5. Write output.
 *
 * @param rangesFile   Path to live-ranges input file.
 * @param configFile   Path to registers/algorithm config file.
 * @param outputFile   Path to output file.
 * @return             The resulting webs (with reg fields populated).
 * @complexity         Dominated by the chosen colouring algorithm.
 */
inline std::vector<Web> runAllocation(const std::string &rangesFile,
                                      const std::string &configFile,
                                      const std::string &outputFile)
{
    // 1. Parse
    auto liveRanges = parseLiveRanges(rangesFile);
    auto cfg        = parseConfig(configFile);

    // 2. Build webs
    std::vector<Web> webs = buildAllWebs(liveRanges);

    // 3. Build interference graph
    Graph<int> G = buildInterferenceGraph(webs);

    // 4. Run algorithm
    bool success = false;

    if (cfg.algorithm == "basic") {
        G.greedyColoring(cfg.numRegisters);
        success = G.getSpilledVertices().empty();

    } else if (cfg.algorithm == "spilling") {
        success = G.allocateWithSpilling(cfg.numRegisters, cfg.algorithmParam);

    } else if (cfg.algorithm == "splitting") {
        success = G.allocateWithSplitting(cfg.numRegisters, cfg.algorithmParam);

    } else if (cfg.algorithm == "free") {
        // "free" mode: try basic first, then spilling with unlimited budget
        G.greedyColoring(cfg.numRegisters);
        if (!G.getSpilledVertices().empty())
            success = G.allocateWithSpilling(cfg.numRegisters,
                                             static_cast<int>(webs.size()));
        else
            success = true;

    } else {
        throw std::runtime_error("Unknown algorithm: " + cfg.algorithm);
    }

    if (!success)
        std::cerr << "WARNING: Could not colour the interference graph with "
                  << cfg.numRegisters << " registers within the given constraints.\n";

    // 5. Copy colours back into the Web structs
    for (auto &w : webs) {
        Vertex<int> *v = G.findVertex(w.webId);
        if (v) w.reg = v->getColor(); // -2 = memory, >=0 = register
    }

    // 6. Write output
    writeOutput(webs, G, outputFile);

    return webs;
}

#endif // REGISTER_ALLOCATOR_H

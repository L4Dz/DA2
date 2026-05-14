#include "da/allocation_output.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <unordered_map>

std::string formatWebPoints(const Web &w) {
    if (w.points.empty())
        return "";
    std::ostringstream oss;
    for (std::size_t i = 0; i < w.points.size(); ++i) {
        if (i > 0)
            oss << ',';
        oss << w.points[i];
        if (i == 0)
            oss << '+';
        else if (i == w.points.size() - 1)
            oss << '-';
    }
    return oss.str();
}

void writeOutput(const std::vector<Web> &webs, const Graph<int> &G, const std::string &outFile) {
    (void)G;

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

    fout << "# Total number of webs followed by the listing of the program points of each one\n";
    fout << "# program points in each web are sorted in ascending order\n";
    fout << "webs: " << webs.size() << '\n';

    for (const auto &w : webs)
        fout << "web" << w.webId << ": " << formatWebPoints(w) << '\n';

    fout << "# Total number of registers used, followed by assignment to webs\n";
    fout << "registers: " << (allSpilled ? 0 : regsUsed) << '\n';

    if (allSpilled) {
        for (const auto &w : webs)
            fout << "M: web" << w.webId << '\n';
    } else {
        std::unordered_map<int, std::vector<int>> regToWebs;
        for (const auto &w : webs) {
            if (w.reg >= 0)
                regToWebs[w.reg].push_back(w.webId);
            else if (w.reg == -2)
                regToWebs[-2].push_back(w.webId);
        }

        for (int r = 0; r < regsUsed; ++r) {
            if (regToWebs.count(r))
                for (int wid : regToWebs[r])
                    fout << "r" << r << ": web" << wid << '\n';
        }
        if (regToWebs.count(-2))
            for (int wid : regToWebs[-2])
                fout << "M: web" << wid << '\n';
    }
}

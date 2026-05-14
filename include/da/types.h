// Domain types for live ranges, webs, and allocator configuration.
#ifndef DA_TYPES_H
#define DA_TYPES_H

#include <string>
#include <vector>

#include "interference.h"

/**
 * @brief One live range for a variable (spec Figure 7).
 */
struct LiveRange {
    std::string          varName;
    std::vector<int>     points;
    std::vector<int>     defPoints;
    std::vector<int>     usePoints;
    bool                 defFirst = true;
    bool                 useLast  = true;

    int start() const { return points.empty() ? -1 : points.front(); }
    int end() const { return points.empty() ? -1 : points.back(); }

    bool overlapsWith(const LiveRange &other) const {
        for (int p : points)
            for (int q : other.points)
                if (p == q)
                    return true;
        return false;
    }
};

/**
 * @brief A web = merged overlapping live ranges of one variable.
 */
struct Web {
    int                  webId = -1;
    std::string          varName;
    std::vector<int>     points;
    std::vector<int>     defPoints;
    std::vector<int>     usePoints;
    bool                 spilled = false;
    int                  reg     = -1;

    int start() const { return points.empty() ? -1 : points.front(); }
    int end() const { return points.empty() ? -1 : points.back(); }

    bool interferesWith(const Web &other) const {
        return programPointsInterfere(points, defPoints, usePoints,
                                      other.points, other.defPoints, other.usePoints);
    }
};

/**
 * @brief Parsed registers / algorithm line from the config file.
 */
struct AllocConfig {
    int         numRegisters   = 0;
    std::string algorithm      = "basic";
    int         algorithmParam = 0;
};

#endif // DA_TYPES_H

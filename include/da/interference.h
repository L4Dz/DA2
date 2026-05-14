// Shared live-range interference test (Project 2 spec §2.5)
#ifndef DA_INTERFERENCE_H
#define DA_INTERFERENCE_H

#include <algorithm>
#include <vector>

/**
 * @brief True iff two live webs (point sets + def/use metadata) interfere.
 * @complexity O(|p1|·|p2|)
 */
inline bool programPointsInterfere(const std::vector<int> &p1,
                                   const std::vector<int> &d1,
                                   const std::vector<int> &u1,
                                   const std::vector<int> &p2,
                                   const std::vector<int> &d2,
                                   const std::vector<int> &u2) {
    if (p1.empty() || p2.empty())
        return false;

    const int start1 = p1.front();
    const int end1   = p1.back();
    const int start2 = p2.front();
    const int end2   = p2.back();

    for (int p : p1) {
        for (int q : p2) {
            if (p != q)
                continue;

            const bool thisEndsUse =
                (p == end1) &&
                (std::find(u1.begin(), u1.end(), p) != u1.end());
            const bool otherStartsDef =
                (p == start2) &&
                (std::find(d2.begin(), d2.end(), p) != d2.end());

            const bool otherEndsUse =
                (p == end2) &&
                (std::find(u2.begin(), u2.end(), p) != u2.end());
            const bool thisStartsDef =
                (p == start1) &&
                (std::find(d1.begin(), d1.end(), p) != d1.end());

            if ((thisEndsUse && otherStartsDef) || (otherEndsUse && thisStartsDef))
                continue;

            return true;
        }
    }
    return false;
}

#endif // DA_INTERFERENCE_H

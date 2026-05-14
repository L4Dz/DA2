#include "da/web_builder.h"

#include <algorithm>

std::vector<Web> buildWebsForVariable(const std::vector<LiveRange> &ranges, int &startId) {
    std::vector<Web> webs;
    for (const auto &lr : ranges) {
        Web w;
        w.varName   = lr.varName;
        w.points    = lr.points;
        w.defPoints = lr.defPoints;
        w.usePoints = lr.usePoints;
        webs.push_back(w);
    }

    bool merged = true;
    while (merged) {
        merged = false;
        for (std::size_t i = 0; i < webs.size() && !merged; ++i) {
            for (std::size_t j = i + 1; j < webs.size(); ++j) {
                bool overlap = false;
                for (int p : webs[i].points) {
                    if (std::find(webs[j].points.begin(), webs[j].points.end(), p) != webs[j].points.end()) {
                        overlap = true;
                        break;
                    }
                }
                if (!overlap)
                    continue;

                for (int p : webs[j].points)
                    if (std::find(webs[i].points.begin(), webs[i].points.end(), p) == webs[i].points.end())
                        webs[i].points.push_back(p);
                for (int p : webs[j].defPoints)
                    if (std::find(webs[i].defPoints.begin(), webs[i].defPoints.end(), p) == webs[i].defPoints.end())
                        webs[i].defPoints.push_back(p);
                for (int p : webs[j].usePoints)
                    if (std::find(webs[i].usePoints.begin(), webs[i].usePoints.end(), p) == webs[i].usePoints.end())
                        webs[i].usePoints.push_back(p);

                std::sort(webs[i].points.begin(), webs[i].points.end());
                std::sort(webs[i].defPoints.begin(), webs[i].defPoints.end());
                std::sort(webs[i].usePoints.begin(), webs[i].usePoints.end());

                webs.erase(webs.begin() + static_cast<std::ptrdiff_t>(j));
                merged = true;
                break;
            }
        }
    }

    for (auto &w : webs)
        w.webId = startId++;
    return webs;
}

std::vector<Web> buildAllWebs(const std::unordered_map<std::string, std::vector<LiveRange>> &allRanges) {
    std::vector<Web> allWebs;
    int nextId = 0;

    std::vector<std::string> varNames;
    for (const auto &kv : allRanges)
        varNames.push_back(kv.first);
    std::sort(varNames.begin(), varNames.end());

    for (const auto &name : varNames) {
        auto webs = buildWebsForVariable(allRanges.at(name), nextId);
        for (auto &w : webs)
            allWebs.push_back(w);
    }

    return allWebs;
}

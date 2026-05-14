#include "da/interference_graph_builder.h"

#include <algorithm>

Graph<int> buildInterferenceGraph(const std::vector<Web> &webs) {
    Graph<int> G;

    for (const auto &w : webs) {
        G.addVertex(w.webId);
        Vertex<int> *v = G.findVertex(w.webId);
        v->setWebId(w.webId);
        v->setProgramPoints(w.points);
        v->setDefPoints(w.defPoints);
        v->setUsePoints(w.usePoints);
        v->addVarName(w.varName);
    }

    for (std::size_t i = 0; i < webs.size(); ++i) {
        for (std::size_t j = i + 1; j < webs.size(); ++j) {
            if (webs[i].interferesWith(webs[j]))
                G.addBidirectionalEdge(webs[i].webId, webs[j].webId, 1.0);
        }
    }

    return G;
}

std::vector<Web> collectWebsFromGraph(const Graph<int> &G) {
    std::vector<const Vertex<int> *> verts;
    verts.reserve(static_cast<std::size_t>(G.getNumVertex()));
    for (auto *v : G.getVertexSet())
        verts.push_back(v);
    std::sort(verts.begin(), verts.end(), [](const Vertex<int> *a, const Vertex<int> *b) {
        return a->getWebId() < b->getWebId();
    });

    std::vector<Web> out;
    out.reserve(verts.size());
    for (const auto *pv : verts) {
        Web w;
        w.webId = pv->getWebId();
        const auto &names = pv->getVarNames();
        w.varName = names.empty() ? std::string() : names.front();
        w.points    = pv->getProgramPoints();
        w.defPoints = pv->getDefPoints();
        w.usePoints = pv->getUsePoints();
        w.reg       = pv->getColor();
        w.spilled   = pv->isSpilled();
        out.push_back(w);
    }
    return out;
}

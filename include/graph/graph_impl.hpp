// Graph<T> template definitions — topology + register colouring heuristics.
#ifndef DA_GRAPH_IMPL_HPP
#define DA_GRAPH_IMPL_HPP

#include <algorithm>
#include <limits>
#include <set>
#include <stack>
#include <vector>

#include "da/interference.h"
#include "vertex_ops.hpp"
#include "graph_decl.hpp"

template <class T>
int Graph<T>::getNumVertex() const { return static_cast<int>(vertexSet.size()); }

template <class T>
std::vector<Vertex<T> *> Graph<T>::getVertexSet() const { return vertexSet; }

template <class T>
Vertex<T> *Graph<T>::findVertex(const T &in) const {
    for (auto v : vertexSet)
        if (v->getInfo() == in) return v;
    return nullptr;
}

template <class T>
int Graph<T>::findVertexIdx(const T &in) const {
    for (unsigned i = 0; i < vertexSet.size(); i++)
        if (vertexSet[i]->getInfo() == in) return static_cast<int>(i);
    return -1;
}

template <class T>
bool Graph<T>::addVertex(const T &in) {
    if (findVertex(in) != nullptr) return false;
    vertexSet.push_back(new Vertex<T>(in));
    return true;
}

template <class T>
bool Graph<T>::removeVertex(const T &in) {
    for (auto it = vertexSet.begin(); it != vertexSet.end(); it++) {
        if ((*it)->getInfo() == in) {
            auto v = *it;
            v->removeOutgoingEdges();
            for (auto u : vertexSet)
                u->removeEdge(v->getInfo());
            vertexSet.erase(it);
            delete v;
            return true;
        }
    }
    return false;
}

template <class T>
bool Graph<T>::addEdge(const T &sourc, const T &dest, double w) {
    auto v1 = findVertex(sourc);
    auto v2 = findVertex(dest);
    if (!v1 || !v2) return false;
    v1->addEdge(v2, w);
    return true;
}

template <class T>
bool Graph<T>::removeEdge(const T &sourc, const T &dest) {
    Vertex<T> *src = findVertex(sourc);
    if (!src) return false;
    return src->removeEdge(dest);
}

template <class T>
bool Graph<T>::addBidirectionalEdge(const T &sourc, const T &dest, double w) {
    auto v1 = findVertex(sourc);
    auto v2 = findVertex(dest);
    if (!v1 || !v2) return false;
    auto e1 = v1->addEdge(v2, w);
    auto e2 = v2->addEdge(v1, w);
    e1->setReverse(e2);
    e2->setReverse(e1);
    return true;
}

template <class T>
void Graph<T>::resetColoringState() {
    for (auto v : vertexSet) {
        if (v->isSpilled()) {
            v->setColor(-2);
            v->setDisabled(true);
            continue;
        }
        v->setColor(-1);
        v->setDisabled(false);
    }
}

template <class T>
void Graph<T>::stripIncidentEdges(Vertex<T> *v) {
    if (!v)
        return;
    const T vi = v->getInfo();
    std::vector<T> neigh;
    for (auto *e : v->getAdj())
        neigh.push_back(e->getDest()->getInfo());
    for (T id : neigh) {
        removeEdge(vi, id);
        removeEdge(id, vi);
    }
}

template <class T>
int Graph<T>::activeVertexCount() const {
    int cnt = 0;
    for (auto v : vertexSet)
        if (!v->isDisabled() && !v->isSpilled()) ++cnt;
    return cnt;
}

template <class T>
int Graph<T>::greedyColoring(int N) {
    resetColoringState();

    std::stack<Vertex<T> *> S;

    while (activeVertexCount() > 0) {
        bool removed = false;

        for (auto v : vertexSet) {
            if (v->isDisabled() || v->isSpilled()) continue;
            if (v->effectiveDegree() < N) {
                v->setDisabled(true);
                S.push(v);
                removed = true;
            }
        }

        if (!removed && activeVertexCount() > 0) {
            Vertex<T> *spill = selectSpillCandidate();
            if (!spill) break;
            spillVertex(spill);
        }
    }

    while (!S.empty()) {
        Vertex<T> *v = S.top();
        S.pop();
        v->setDisabled(false);

        std::set<int> usedColors;
        for (auto e : v->getAdj()) {
            Vertex<T> *nb = e->getDest();
            if (!nb->isSpilled() && nb->getColor() >= 0)
                usedColors.insert(nb->getColor());
        }

        int c = 0;
        while (c < N && usedColors.count(c))
            ++c;
        if (c >= N) {
            spillVertex(v);
            continue;
        }
        v->setColor(c);
    }

    return chromaticNumber();
}

template <class T>
int Graph<T>::greedyColoringDSatur(int N) {
    resetColoringState();

    auto active = [](Vertex<T> *v) { return !v->isSpilled(); };

    while (true) {
        Vertex<T> *best = nullptr;
        int bestSat = -1;
        int bestDeg = -1;
        int bestWid = std::numeric_limits<int>::max();

        for (auto v : vertexSet) {
            if (!active(v) || v->getColor() >= 0)
                continue;

            std::set<int> sat;
            int deg = 0;
            for (auto e : v->getAdj()) {
                Vertex<T> *nb = e->getDest();
                if (!active(nb))
                    continue;
                ++deg;
                if (nb->getColor() >= 0)
                    sat.insert(nb->getColor());
            }

            const int satCount = static_cast<int>(sat.size());
            const int wid = v->getWebId();

            if (satCount > bestSat ||
                (satCount == bestSat && deg > bestDeg) ||
                (satCount == bestSat && deg == bestDeg && wid < bestWid)) {
                best    = v;
                bestSat = satCount;
                bestDeg = deg;
                bestWid = wid;
            }
        }

        if (!best)
            break;

        std::set<int> used;
        for (auto e : best->getAdj()) {
            Vertex<T> *nb = e->getDest();
            if (active(nb) && nb->getColor() >= 0)
                used.insert(nb->getColor());
        }

        int c = 0;
        while (c < N && used.count(c))
            ++c;

        if (c >= N) {
            spillVertex(best);
            continue;
        }

        best->setColor(c);
    }

    return chromaticNumber();
}

template <class T>
Vertex<T> *Graph<T>::selectSpillCandidate() {
    Vertex<T> *best = nullptr;
    int bestDeg = -1;
    for (auto v : vertexSet) {
        if (v->isDisabled() || v->isSpilled()) continue;
        int d = v->effectiveDegree();
        if (d > bestDeg || (d == bestDeg && (best == nullptr || v->getWebId() < best->getWebId()))) {
            bestDeg = d;
            best = v;
        }
    }
    return best;
}

template <class T>
void Graph<T>::spillVertex(Vertex<T> *v) {
    v->setSpilled(true);
    v->setDisabled(true);
    v->setColor(-2);
}

template <class T>
bool Graph<T>::allocateWithSpilling(int N, int maxSpills) {
    int spillsUsed = 0;

    for (auto v : vertexSet) v->setSpilled(false);

    while (true) {
        greedyColoring(N);

        int forcedSpills = 0;
        for (auto v : vertexSet)
            if (v->getColor() == -2) ++forcedSpills;

        if (forcedSpills == 0) return true;

        if (spillsUsed + forcedSpills > maxSpills) return false;

        spillsUsed += forcedSpills;
    }
}

template <class T>
Vertex<T> *Graph<T>::splitWeb(Vertex<T> *v, std::vector<Vertex<T> *> &allWebs) {
    const auto &pts = v->getProgramPoints();
    if (pts.size() < 2)
        return nullptr;

    std::vector<int> sortedPts = pts;
    std::sort(sortedPts.begin(), sortedPts.end());

    const std::size_t mid = sortedPts.size() / 2;
    std::vector<int> lowerHalf(sortedPts.begin(), sortedPts.begin() + static_cast<std::ptrdiff_t>(mid));
    std::vector<int> upperHalf(sortedPts.begin() + static_cast<std::ptrdiff_t>(mid), sortedPts.end());

    auto hasPoint = [](const std::vector<int> &s, int x) {
        return std::find(s.begin(), s.end(), x) != s.end();
    };

    std::vector<int> lowerDef, upperDef, lowerUse, upperUse;
    for (int d : v->getDefPoints()) {
        if (hasPoint(lowerHalf, d))
            lowerDef.push_back(d);
        else if (hasPoint(upperHalf, d))
            upperDef.push_back(d);
    }
    for (int u : v->getUsePoints()) {
        if (hasPoint(lowerHalf, u))
            lowerUse.push_back(u);
        else if (hasPoint(upperHalf, u))
            upperUse.push_back(u);
    }
    std::sort(lowerDef.begin(), lowerDef.end());
    std::sort(upperDef.begin(), upperDef.end());
    std::sort(lowerUse.begin(), lowerUse.end());
    std::sort(upperUse.begin(), upperUse.end());

    v->setSpilled(false);
    v->setDisabled(false);
    v->setColor(-1);

    stripIncidentEdges(v);

    v->setProgramPoints(lowerHalf);
    v->setDefPoints(std::move(lowerDef));
    v->setUsePoints(std::move(lowerUse));

    int maxInfo = -1;
    for (auto *u : vertexSet)
        maxInfo = std::max(maxInfo, static_cast<int>(u->getInfo()));
    const int newInfoInt = maxInfo + 1;
    const T newInfo = static_cast<T>(newInfoInt);

    addVertex(newInfo);
    Vertex<T> *derived = findVertex(newInfo);
    derived->setWebId(newInfoInt);
    derived->setProgramPoints(upperHalf);
    derived->setDefPoints(std::move(upperDef));
    derived->setUsePoints(std::move(upperUse));
    derived->setVarNames(v->getVarNames());

    for (auto *other : vertexSet) {
        if (other == v || other == derived || other->isSpilled())
            continue;

        if (programPointsInterfere(v->getProgramPoints(), v->getDefPoints(), v->getUsePoints(),
                                   other->getProgramPoints(), other->getDefPoints(), other->getUsePoints()))
            addBidirectionalEdge(v->getInfo(), other->getInfo(), 1.0);

        if (programPointsInterfere(derived->getProgramPoints(), derived->getDefPoints(), derived->getUsePoints(),
                                   other->getProgramPoints(), other->getDefPoints(), other->getUsePoints()))
            addBidirectionalEdge(derived->getInfo(), other->getInfo(), 1.0);
    }

    allWebs.push_back(derived);
    return derived;
}

template <class T>
bool Graph<T>::allocateWithSplitting(int N, int maxSplits) {
    for (auto *v : vertexSet)
        v->setSpilled(false);

    std::vector<Vertex<T> *> allWebs(vertexSet.begin(), vertexSet.end());
    int splitsUsed = 0;

    while (true) {
        greedyColoring(N);

        auto spills = getSpilledVertices();
        if (spills.empty())
            return true;
        if (splitsUsed >= maxSplits)
            return false;

        Vertex<T> *candidate = nullptr;
        int bestDeg = -1;
        int bestWid = std::numeric_limits<int>::max();
        for (auto *sv : spills) {
            int deg = 0;
            for (auto *e : sv->getAdj()) {
                Vertex<T> *nb = e->getDest();
                if (!nb->isSpilled())
                    ++deg;
            }
            const int wid = sv->getWebId();
            if (deg > bestDeg || (deg == bestDeg && wid < bestWid)) {
                bestDeg   = deg;
                bestWid   = wid;
                candidate = sv;
            }
        }

        if (!candidate)
            return false;

        Vertex<T> *derived = splitWeb(candidate, allWebs);
        if (!derived)
            return false;
        ++splitsUsed;
    }
}

template <class T>
bool Graph<T>::interferes(Vertex<T> *u, Vertex<T> *v) const {
    for (auto e : u->getAdj())
        if (e->getDest() == v) return true;
    return false;
}

template <class T>
std::vector<Vertex<T> *> Graph<T>::getSpilledVertices() const {
    std::vector<Vertex<T> *> result;
    for (auto v : vertexSet)
        if (v->getColor() == -2) result.push_back(v);
    return result;
}

template <class T>
std::vector<Vertex<T> *> Graph<T>::getColoredVertices() const {
    std::vector<Vertex<T> *> result;
    for (auto v : vertexSet)
        if (v->getColor() >= 0) result.push_back(v);
    return result;
}

template <class T>
int Graph<T>::chromaticNumber() const {
    int maxColor = -1;
    for (auto v : vertexSet)
        if (v->getColor() >= 0 && v->getColor() > maxColor)
            maxColor = v->getColor();
    return maxColor + 1;
}

template <class T>
Graph<T>::~Graph() {
    deleteMatrix(distMatrix, static_cast<int>(vertexSet.size()));
    deleteMatrix(pathMatrix, static_cast<int>(vertexSet.size()));
    for (auto v : vertexSet) delete v;
}

inline void deleteMatrix(int **m, int n) {
    if (m) {
        for (int i = 0; i < n; i++) delete[] m[i];
        delete[] m;
    }
}

inline void deleteMatrix(double **m, int n) {
    if (m) {
        for (int i = 0; i < n; i++) delete[] m[i];
        delete[] m;
    }
}

#endif // DA_GRAPH_IMPL_HPP

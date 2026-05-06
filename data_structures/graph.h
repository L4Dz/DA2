// Original code by Gonçalo Leão
// Updated by DA 2024/2025 Team
// Modified for Project 2 - Compiler Register Allocation (Spring 2026)

#ifndef DA_TP_CLASSES_GRAPH
#define DA_TP_CLASSES_GRAPH

#include <iostream>
#include <vector>
#include <queue>
#include <stack>
#include <limits>
#include <algorithm>
#include <set>
#include <unordered_map>
#include <string>
#include "MutablePriorityQueue.h"

template <class T>
class Edge;

#define INF std::numeric_limits<double>::max()

/************************* Vertex  **************************/

template <class T>
class Vertex {
public:
    Vertex(T in);
    bool operator<(Vertex<T> & vertex) const;

    // ── original getters ──────────────────────────────────────
    T getInfo() const;
    std::vector<Edge<T> *> getAdj() const;
    bool isVisited() const;
    bool isProcessing() const;
    unsigned int getIndegree() const;
    double getDist() const;
    Edge<T> *getPath() const;
    std::vector<Edge<T> *> getIncoming() const;

    // ── original setters ──────────────────────────────────────
    void setInfo(T info);
    void setVisited(bool visited);
    void setProcessing(bool processing);
    void setIndegree(unsigned int indegree);
    void setDist(double dist);
    void setPath(Edge<T> *path);

    // ── SCC / topsort helpers (kept from original) ────────────
    int getLow() const;
    void setLow(int value);
    int getNum() const;
    void setNum(int value);

    // ── edge helpers ──────────────────────────────────────────
    Edge<T> *addEdge(Vertex<T> *dest, double w);
    bool removeEdge(T in);
    void removeOutgoingEdges();

    // =========================================================
    // PROJECT-SPECIFIC: register allocation / graph colouring
    // =========================================================

    /**
     * @brief Web identifier assigned during web construction.
     *        Each vertex in the interference graph represents one web.
     *        Default -1 means "not yet assigned".
     * @complexity O(1)
     */
    int getWebId() const { return webId; }
    void setWebId(int id) { webId = id; }

    /**
     * @brief The colour (register number) assigned by the colouring algorithm.
     *        -1 = not yet coloured.   -2 = spilled (assigned to memory).
     * @complexity O(1)
     */
    int getColor() const { return color; }
    void setColor(int c) { color = c; }

    /**
     * @brief Whether this vertex has been "disabled" (logically removed)
     *        during the simplification phase of the greedy colouring algorithm.
     *        A disabled vertex is not counted when computing neighbour degrees.
     * @complexity O(1)
     */
    bool isDisabled() const { return disabled; }
    void setDisabled(bool d) { disabled = d; }

    /**
     * @brief Whether this web has been selected for spilling (committed to memory).
     *        Spilled webs are removed from the interference graph before colouring.
     * @complexity O(1)
     */
    bool isSpilled() const { return spilled; }
    void setSpilled(bool s) { spilled = s; }

    /**
     * @brief The variable name(s) this web is associated with.
     *        A web can cover one or more original variable names after web splitting.
     */
    const std::vector<std::string>& getVarNames() const { return varNames; }
    void addVarName(const std::string& name) { varNames.push_back(name); }
    void setVarNames(const std::vector<std::string>& names) { varNames = names; }

    /**
     * @brief The sorted list of program points (line numbers) that belong to this web.
     *        Format mirrors the input/output specification:
     *          first entry has '+' (definition), last has '-' (last use).
     *        Stored as raw integers; the '+'/'-' annotation is managed by the I/O layer.
     * @complexity O(1)
     */
    const std::vector<int>& getProgramPoints() const { return programPoints; }
    void setProgramPoints(const std::vector<int>& pts) { programPoints = pts; }
    void addProgramPoint(int pt) { programPoints.push_back(pt); }

    /**
     * @brief Effective degree = number of non-disabled neighbours.
     *        Used by the greedy colouring algorithm instead of adj.size().
     * @complexity O(degree)
     */
    int effectiveDegree() const;

    friend class MutablePriorityQueue<Vertex>;

protected:
    T info;
    std::vector<Edge<T> *> adj;

    // ── original auxiliary fields ─────────────────────────────
    bool visited    = false;
    bool processing = false;
    int  low = -1, num = -1;
    unsigned int indegree = 0;
    double dist = 0;
    Edge<T> *path = nullptr;
    std::vector<Edge<T> *> incoming;
    int queueIndex = 0;

    // ── project-specific fields ───────────────────────────────
    int  webId   = -1;   ///< web identifier (index in the interference graph)
    int  color   = -1;   ///< assigned register (-1 = none, -2 = spilled)
    bool disabled = false; ///< logically removed during simplification
    bool spilled  = false; ///< marked for spilling before colouring

    std::vector<std::string> varNames;     ///< variable names this web covers
    std::vector<int>         programPoints; ///< program-point line numbers

    void deleteEdge(Edge<T> *edge);
};

/********************** Edge  ****************************/

template <class T>
class Edge {
public:
    Edge(Vertex<T> *orig, Vertex<T> *dest, double w);

    Vertex<T> *getDest()    const;
    double      getWeight() const;
    bool        isSelected() const;
    Vertex<T> *getOrig()    const;
    Edge<T>   *getReverse() const;
    double      getFlow()   const;

    void setSelected(bool selected);
    void setReverse(Edge<T> *reverse);
    void setFlow(double flow);

protected:
    Vertex<T> *dest;
    double     weight;
    bool       selected = false;
    Vertex<T> *orig;
    Edge<T>   *reverse  = nullptr;
    double     flow      = 0;
};

/********************** Graph  ****************************/

template <class T>
class Graph {
public:
    ~Graph();

    // ── vertex / edge management (original) ──────────────────
    Vertex<T> *findVertex(const T &in) const;
    bool addVertex(const T &in);
    bool removeVertex(const T &in);
    bool addEdge(const T &sourc, const T &dest, double w);
    bool removeEdge(const T &source, const T &dest);
    bool addBidirectionalEdge(const T &sourc, const T &dest, double w);
    int  getNumVertex() const;
    std::vector<Vertex<T> *> getVertexSet() const;

    // =========================================================
    // PROJECT-SPECIFIC: interference graph & register allocation
    // =========================================================

    /**
     * @brief Reset the disabled/colour flags on all vertices so the graph
     *        can be reused for a new colouring attempt.
     * @complexity O(V)
     */
    void resetColoringState();

    /**
     * @brief Return the number of active (non-disabled, non-spilled) vertices.
     * @complexity O(V)
     */
    int activeVertexCount() const;

    /**
     * @brief Greedy graph-colouring algorithm (Figure 8 of the project spec).
     *
     *  Phase 1 – Simplification: repeatedly remove vertices whose effective
     *             degree < N and push them onto a stack.  If all remaining
     *             vertices have degree >= N, a vertex is selected for spilling
     *             and removed (marked disabled, NOT pushed onto the stack).
     *
     *  Phase 2 – Colouring: pop vertices from the stack and assign the lowest
     *             colour not used by any of their (already-coloured) neighbours.
     *
     * @param N   Number of available registers (colours).
     * @return    Number of colours actually used (≤ N), or -1 if colouring
     *            failed (should not happen if spilling is allowed).
     * @complexity O(V * (V + E))  – typical greedy, not optimal
     */
    int greedyColoring(int N);

    /**
     * @brief Select the best candidate vertex to spill during simplification.
     *
     *  Heuristic: prefer the vertex with the highest effective degree
     *  (removing it relieves the most constraints).  Ties broken by webId.
     *
     * @return Pointer to the chosen vertex, or nullptr if none available.
     * @complexity O(V)
     */
    Vertex<T> *selectSpillCandidate();

    /**
     * @brief Mark a web as spilled: set its spilled flag, disable it in the
     *        graph, and assign colour -2 (memory).
     * @param v  The vertex (web) to spill.
     * @complexity O(1)
     */
    void spillVertex(Vertex<T> *v);

    /**
     * @brief Attempt register allocation with web spilling.
     *
     *  Tries the basic greedy algorithm first.  If it fails (forced spills
     *  occur), iteratively spills the best candidate until the graph is
     *  colourable with N registers or the maximum number of allowed spills
     *  (maxSpills) is reached.
     *
     * @param N          Number of available registers.
     * @param maxSpills  Maximum number of webs that may be spilled.
     * @return true if a valid colouring was found within the spill budget.
     * @complexity O(maxSpills * V * (V + E))
     */
    bool allocateWithSpilling(int N, int maxSpills);

    /**
     * @brief Split a web (vertex) into two derived webs.
     *
     *  The original vertex's program points are split at the midpoint.
     *  A new vertex is inserted into the graph, interference edges are
     *  recomputed for the new vertex, and the original vertex's adjacency
     *  list is updated.
     *
     * @param v          The vertex (web) to split.
     * @param allWebs    Reference to all webs so interference can be recomputed.
     * @return Pointer to the newly created derived web vertex.
     * @complexity O(V + E)
     */
    Vertex<T> *splitWeb(Vertex<T> *v, std::vector<Vertex<T> *> &allWebs);

    /**
     * @brief Attempt register allocation with web splitting.
     *
     *  Mirrors allocateWithSpilling but uses web splitting instead of spilling.
     *
     * @param N          Number of available registers.
     * @param maxSplits  Maximum number of webs that may be split.
     * @return true if a valid colouring was found within the split budget.
     * @complexity O(maxSplits * V * (V + E))
     */
    bool allocateWithSplitting(int N, int maxSplits);

    /**
     * @brief Check whether two vertices (webs) interfere, i.e., whether an
     *        edge exists between them in the interference graph.
     * @complexity O(degree(u))
     */
    bool interferes(Vertex<T> *u, Vertex<T> *v) const;

    /**
     * @brief Return all vertices that have been assigned to memory (color == -2).
     * @complexity O(V)
     */
    std::vector<Vertex<T> *> getSpilledVertices() const;

    /**
     * @brief Return all vertices that have been successfully coloured (color >= 0).
     * @complexity O(V)
     */
    std::vector<Vertex<T> *> getColoredVertices() const;

    /**
     * @brief Return the chromatic number achieved by the last colouring run,
     *        i.e., 1 + max colour index used.
     * @complexity O(V)
     */
    int chromaticNumber() const;

protected:
    std::vector<Vertex<T> *> vertexSet;
    double **distMatrix = nullptr;
    int    **pathMatrix = nullptr;

    int findVertexIdx(const T &in) const;
};

// ── free helpers ──────────────────────────────────────────────────────────────
void deleteMatrix(int    **m, int n);
void deleteMatrix(double **m, int n);

// =============================================================================
//  IMPLEMENTATION
// =============================================================================

/************************* Vertex  **************************/

template <class T>
Vertex<T>::Vertex(T in) : info(in) {}

template <class T>
Edge<T> *Vertex<T>::addEdge(Vertex<T> *d, double w) {
    auto newEdge = new Edge<T>(this, d, w);
    adj.push_back(newEdge);
    d->incoming.push_back(newEdge);
    return newEdge;
}

template <class T>
bool Vertex<T>::removeEdge(T in) {
    bool removedEdge = false;
    auto it = adj.begin();
    while (it != adj.end()) {
        Edge<T> *edge = *it;
        Vertex<T> *dest = edge->getDest();
        if (dest->getInfo() == in) {
            it = adj.erase(it);
            deleteEdge(edge);
            removedEdge = true;
        } else {
            it++;
        }
    }
    return removedEdge;
}

template <class T>
void Vertex<T>::removeOutgoingEdges() {
    auto it = adj.begin();
    while (it != adj.end()) {
        Edge<T> *edge = *it;
        it = adj.erase(it);
        deleteEdge(edge);
    }
}

template <class T>
bool Vertex<T>::operator<(Vertex<T> &vertex) const {
    return this->dist < vertex.dist;
}

template <class T>
int Vertex<T>::effectiveDegree() const {
    int deg = 0;
    for (auto e : adj) {
        Vertex<T> *nb = e->getDest();
        if (!nb->isDisabled() && !nb->isSpilled())
            ++deg;
    }
    return deg;
}

// ── trivial getters / setters (unchanged from original) ──────────────────────

template <class T> T                       Vertex<T>::getInfo()       const { return info; }
template <class T> int                     Vertex<T>::getLow()        const { return low; }
template <class T> void                    Vertex<T>::setLow(int v)         { low = v; }
template <class T> int                     Vertex<T>::getNum()        const { return num; }
template <class T> void                    Vertex<T>::setNum(int v)         { num = v; }
template <class T> std::vector<Edge<T>*>   Vertex<T>::getAdj()        const { return adj; }
template <class T> bool                    Vertex<T>::isVisited()     const { return visited; }
template <class T> bool                    Vertex<T>::isProcessing()  const { return processing; }
template <class T> unsigned int            Vertex<T>::getIndegree()   const { return indegree; }
template <class T> double                  Vertex<T>::getDist()       const { return dist; }
template <class T> Edge<T>*                Vertex<T>::getPath()       const { return path; }
template <class T> std::vector<Edge<T>*>   Vertex<T>::getIncoming()   const { return incoming; }
template <class T> void                    Vertex<T>::setInfo(T in)         { info = in; }
template <class T> void                    Vertex<T>::setVisited(bool v)    { visited = v; }
template <class T> void                    Vertex<T>::setProcessing(bool p) { processing = p; }
template <class T> void                    Vertex<T>::setIndegree(unsigned int i) { indegree = i; }
template <class T> void                    Vertex<T>::setDist(double d)     { dist = d; }
template <class T> void                    Vertex<T>::setPath(Edge<T>*p)    { path = p; }

template <class T>
void Vertex<T>::deleteEdge(Edge<T> *edge) {
    Vertex<T> *dest = edge->getDest();
    auto it = dest->incoming.begin();
    while (it != dest->incoming.end()) {
        if ((*it)->getOrig()->getInfo() == info)
            it = dest->incoming.erase(it);
        else
            it++;
    }
    delete edge;
}

/********************** Edge  ****************************/

template <class T>
Edge<T>::Edge(Vertex<T> *orig, Vertex<T> *dest, double w)
    : orig(orig), dest(dest), weight(w) {}

template <class T> Vertex<T>* Edge<T>::getDest()    const { return dest; }
template <class T> double      Edge<T>::getWeight()  const { return weight; }
template <class T> Vertex<T>* Edge<T>::getOrig()    const { return orig; }
template <class T> Edge<T>*   Edge<T>::getReverse() const { return reverse; }
template <class T> bool        Edge<T>::isSelected() const { return selected; }
template <class T> double      Edge<T>::getFlow()    const { return flow; }
template <class T> void        Edge<T>::setSelected(bool s)      { selected = s; }
template <class T> void        Edge<T>::setReverse(Edge<T>* r)   { reverse = r; }
template <class T> void        Edge<T>::setFlow(double f)         { flow = f; }

/********************** Graph  ****************************/

template <class T>
int Graph<T>::getNumVertex() const { return vertexSet.size(); }

template <class T>
std::vector<Vertex<T>*> Graph<T>::getVertexSet() const { return vertexSet; }

template <class T>
Vertex<T>* Graph<T>::findVertex(const T &in) const {
    for (auto v : vertexSet)
        if (v->getInfo() == in) return v;
    return nullptr;
}

template <class T>
int Graph<T>::findVertexIdx(const T &in) const {
    for (unsigned i = 0; i < vertexSet.size(); i++)
        if (vertexSet[i]->getInfo() == in) return i;
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

// ── project-specific methods ──────────────────────────────────────────────────

template <class T>
void Graph<T>::resetColoringState() {
    for (auto v : vertexSet) {
        v->setColor(-1);
        v->setDisabled(false);
        // Note: spilled flag is intentionally NOT reset here so that
        // previously spilled webs remain excluded across re-colouring attempts.
    }
}

template <class T>
int Graph<T>::activeVertexCount() const {
    int cnt = 0;
    for (auto v : vertexSet)
        if (!v->isDisabled() && !v->isSpilled()) ++cnt;
    return cnt;
}

/**
 * @brief Greedy graph-colouring (Figure 8 of the spec).
 *
 * Phase 1 – Simplification loop:
 *   While any active vertex has effectiveDegree < N, remove it to the stack.
 *   If stuck (all degrees >= N), pick a spill candidate, mark it disabled
 *   (NOT pushed onto the stack – it receives no colour assignment).
 *
 * Phase 2 – Colouring loop:
 *   Pop each vertex from the stack, collect colours of active neighbours,
 *   and assign the lowest colour not in that set.
 *
 * @complexity O(V² + V·E) worst-case
 */
template <class T>
int Graph<T>::greedyColoring(int N) {
    resetColoringState();

    std::stack<Vertex<T>*> S;

    // ── Phase 1: simplification ───────────────────────────────────────────────
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
            // All remaining vertices have degree >= N: force-spill one.
            Vertex<T> *spill = selectSpillCandidate();
            if (!spill) break; // safety guard
            spillVertex(spill);
        }
    }

    // ── Phase 2: colouring ────────────────────────────────────────────────────
    // Re-enable vertices in reverse simplification order and assign colours.
    while (!S.empty()) {
        Vertex<T> *v = S.top(); S.pop();
        v->setDisabled(false);

        // Collect colours already used by neighbours (active or already coloured).
        std::set<int> usedColors;
        for (auto e : v->getAdj()) {
            Vertex<T> *nb = e->getDest();
            if (!nb->isSpilled() && nb->getColor() >= 0)
                usedColors.insert(nb->getColor());
        }

        // Assign lowest available colour.
        int c = 0;
        while (usedColors.count(c)) ++c;
        v->setColor(c);
    }

    return chromaticNumber();
}

/**
 * @brief Select the vertex to spill: highest effective degree wins.
 * @complexity O(V)
 */
template <class T>
Vertex<T>* Graph<T>::selectSpillCandidate() {
    Vertex<T> *best = nullptr;
    int bestDeg = -1;
    for (auto v : vertexSet) {
        if (v->isDisabled() || v->isSpilled()) continue;
        int d = v->effectiveDegree();
        if (d > bestDeg || (d == bestDeg && best && v->getWebId() < best->getWebId())) {
            bestDeg = d;
            best = v;
        }
    }
    return best;
}

/**
 * @complexity O(1)
 */
template <class T>
void Graph<T>::spillVertex(Vertex<T> *v) {
    v->setSpilled(true);
    v->setDisabled(true);
    v->setColor(-2); // -2 ≡ memory
}

/**
 * @complexity O(maxSpills · V · (V + E))
 */
template <class T>
bool Graph<T>::allocateWithSpilling(int N, int maxSpills) {
    int spillsUsed = 0;

    // Reset spill flags before starting.
    for (auto v : vertexSet) v->setSpilled(false);

    while (true) {
        greedyColoring(N);

        // Count forced spills produced by this run.
        int forcedSpills = 0;
        for (auto v : vertexSet)
            if (v->getColor() == -2) ++forcedSpills;

        if (forcedSpills == 0) return true; // success

        if (spillsUsed + forcedSpills > maxSpills) return false; // budget exceeded

        spillsUsed += forcedSpills;
        // Forced-spilled vertices are already marked; next greedyColoring run
        // will exclude them automatically.
    }
}

/**
 * @brief Split a web at its midpoint and insert the derived web into the graph.
 *
 *  The lower half of program points stays with the original vertex.
 *  The upper half goes to the new vertex.
 *  Interference edges for the new vertex are added against any web whose
 *  program-point range overlaps with the new web's range.
 *
 * @complexity O(V + E)
 */
template <class T>
Vertex<T>* Graph<T>::splitWeb(Vertex<T> *v, std::vector<Vertex<T>*> &allWebs) {
    auto &pts = v->getProgramPoints();
    if (pts.size() < 2) return nullptr; // cannot split a single-point web

    std::vector<int> sortedPts = pts;
    std::sort(sortedPts.begin(), sortedPts.end());

    std::size_t mid = sortedPts.size() / 2;
    std::vector<int> lowerHalf(sortedPts.begin(), sortedPts.begin() + mid);
    std::vector<int> upperHalf(sortedPts.begin() + mid, sortedPts.end());

    // Update original vertex to keep lower half.
    v->setProgramPoints(lowerHalf);

    // Create the derived vertex with the upper half.
    // We use webId = vertexSet.size() as a unique identifier.
    int newId = static_cast<int>(vertexSet.size());
    T newInfo = static_cast<T>(newId); // works when T = int (web index)
    addVertex(newInfo);
    Vertex<T> *derived = findVertex(newInfo);
    derived->setWebId(newId);
    derived->setProgramPoints(upperHalf);
    derived->setVarNames(v->getVarNames()); // same variable family

    // Recompute interference: add edges between derived and any overlapping web.
    for (auto other : vertexSet) {
        if (other == derived || other->isSpilled()) continue;

        // Two webs interfere if their program-point sets share at least one point.
        const auto &otherPts = other->getProgramPoints();
        bool overlap = false;
        for (int p : upperHalf) {
            if (std::find(otherPts.begin(), otherPts.end(), p) != otherPts.end()) {
                overlap = true;
                break;
            }
        }
        if (overlap) {
            addBidirectionalEdge(derived->getInfo(), other->getInfo(), 1.0);
        }
    }

    allWebs.push_back(derived);
    return derived;
}

/**
 * @complexity O(maxSplits · V · (V + E))
 */
template <class T>
bool Graph<T>::allocateWithSplitting(int N, int maxSplits) {
    std::vector<Vertex<T>*> allWebs(vertexSet.begin(), vertexSet.end());
    int splitsUsed = 0;

    while (true) {
        greedyColoring(N);

        // Check if any vertex was force-spilled (meaning colouring failed).
        bool failed = false;
        Vertex<T> *candidate = nullptr;
        int bestDeg = -1;

        for (auto v : vertexSet) {
            if (v->getColor() == -2 && !v->isSpilled()) {
                failed = true;
                int d = v->effectiveDegree();
                if (d > bestDeg) { bestDeg = d; candidate = v; }
            }
        }

        if (!failed) return true;
        if (splitsUsed >= maxSplits) return false;

        if (!candidate) return false;
        splitWeb(candidate, allWebs);
        ++splitsUsed;
    }
}

/**
 * @complexity O(degree(u))
 */
template <class T>
bool Graph<T>::interferes(Vertex<T> *u, Vertex<T> *v) const {
    for (auto e : u->getAdj())
        if (e->getDest() == v) return true;
    return false;
}

/**
 * @complexity O(V)
 */
template <class T>
std::vector<Vertex<T>*> Graph<T>::getSpilledVertices() const {
    std::vector<Vertex<T>*> result;
    for (auto v : vertexSet)
        if (v->getColor() == -2) result.push_back(v);
    return result;
}

/**
 * @complexity O(V)
 */
template <class T>
std::vector<Vertex<T>*> Graph<T>::getColoredVertices() const {
    std::vector<Vertex<T>*> result;
    for (auto v : vertexSet)
        if (v->getColor() >= 0) result.push_back(v);
    return result;
}

/**
 * @complexity O(V)
 */
template <class T>
int Graph<T>::chromaticNumber() const {
    int maxColor = -1;
    for (auto v : vertexSet)
        if (v->getColor() > maxColor) maxColor = v->getColor();
    return maxColor + 1; // number of distinct colours used
}

template <class T>
Graph<T>::~Graph() {
    deleteMatrix(distMatrix, vertexSet.size());
    deleteMatrix(pathMatrix, vertexSet.size());
    for (auto v : vertexSet) delete v;
}

// ── matrix helpers ────────────────────────────────────────────────────────────

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

#endif /* DA_TP_CLASSES_GRAPH */

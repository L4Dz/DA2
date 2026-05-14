// Graph class declaration (TP course structure + register-allocation API).
#ifndef DA_GRAPH_DECL_HPP
#define DA_GRAPH_DECL_HPP

#include <vector>

template <class T>
class Vertex;

template <class T>
class Graph {
public:
    ~Graph();

    Vertex<T> *findVertex(const T &in) const;
    bool addVertex(const T &in);
    bool removeVertex(const T &in);
    bool addEdge(const T &sourc, const T &dest, double w);
    bool removeEdge(const T &source, const T &dest);
    bool addBidirectionalEdge(const T &sourc, const T &dest, double w);
    int  getNumVertex() const;
    std::vector<Vertex<T> *> getVertexSet() const;

    void resetColoringState();
    int activeVertexCount() const;

    int greedyColoring(int N);
    int greedyColoringDSatur(int N);

    Vertex<T> *selectSpillCandidate();
    void spillVertex(Vertex<T> *v);

    bool allocateWithSpilling(int N, int maxSpills);

    Vertex<T> *splitWeb(Vertex<T> *v, std::vector<Vertex<T> *> &allWebs);
    void stripIncidentEdges(Vertex<T> *v);

    bool allocateWithSplitting(int N, int maxSplits);

    bool interferes(Vertex<T> *u, Vertex<T> *v) const;

    std::vector<Vertex<T> *> getSpilledVertices() const;
    std::vector<Vertex<T> *> getColoredVertices() const;
    int chromaticNumber() const;

protected:
    std::vector<Vertex<T> *> vertexSet;
    double **distMatrix = nullptr;
    int    **pathMatrix = nullptr;

    int findVertexIdx(const T &in) const;
};

void deleteMatrix(int **m, int n);
void deleteMatrix(double **m, int n);

#endif // DA_GRAPH_DECL_HPP

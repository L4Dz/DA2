#ifndef DA_GRAPH_VERTEX_OPS_HPP
#define DA_GRAPH_VERTEX_OPS_HPP

#include "vertex_decl.hpp"
#include "edge.hpp"

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

#endif // DA_GRAPH_VERTEX_OPS_HPP

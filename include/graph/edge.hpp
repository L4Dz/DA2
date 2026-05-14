#ifndef DA_GRAPH_EDGE_HPP
#define DA_GRAPH_EDGE_HPP

#include "vertex_decl.hpp"

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

#endif // DA_GRAPH_EDGE_HPP

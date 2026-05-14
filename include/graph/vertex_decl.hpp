#ifndef DA_GRAPH_VERTEX_DECL_HPP
#define DA_GRAPH_VERTEX_DECL_HPP

#include <string>
#include <vector>

#include "MutablePriorityQueue.h"

template <class T>
class Edge;

template <class T>
class Vertex {
public:
    Vertex(T in);
    bool operator<(Vertex<T> &vertex) const;

    T getInfo() const;
    std::vector<Edge<T> *> getAdj() const;
    bool isVisited() const;
    bool isProcessing() const;
    unsigned int getIndegree() const;
    double getDist() const;
    Edge<T> *getPath() const;
    std::vector<Edge<T> *> getIncoming() const;

    void setInfo(T info);
    void setVisited(bool visited);
    void setProcessing(bool processing);
    void setIndegree(unsigned int indegree);
    void setDist(double dist);
    void setPath(Edge<T> *path);

    int getLow() const;
    void setLow(int value);
    int getNum() const;
    void setNum(int value);

    Edge<T> *addEdge(Vertex<T> *dest, double w);
    bool removeEdge(T in);
    void removeOutgoingEdges();

    int getWebId() const { return webId; }
    void setWebId(int id) { webId = id; }

    int getColor() const { return color; }
    void setColor(int c) { color = c; }

    bool isDisabled() const { return disabled; }
    void setDisabled(bool d) { disabled = d; }

    bool isSpilled() const { return spilled; }
    void setSpilled(bool s) { spilled = s; }

    const std::vector<std::string> &getVarNames() const { return varNames; }
    void addVarName(const std::string &name) { varNames.push_back(name); }
    void setVarNames(const std::vector<std::string> &names) { varNames = names; }

    const std::vector<int> &getProgramPoints() const { return programPoints; }
    void setProgramPoints(const std::vector<int> &pts) { programPoints = pts; }
    void addProgramPoint(int pt) { programPoints.push_back(pt); }

    const std::vector<int> &getDefPoints() const { return defPoints; }
    void setDefPoints(std::vector<int> pts) { defPoints = std::move(pts); }

    const std::vector<int> &getUsePoints() const { return usePoints; }
    void setUsePoints(std::vector<int> pts) { usePoints = std::move(pts); }

    int effectiveDegree() const;

    friend class MutablePriorityQueue<Vertex>;

protected:
    T info;
    std::vector<Edge<T> *> adj;

    bool visited    = false;
    bool processing = false;
    int  low = -1, num = -1;
    unsigned int indegree = 0;
    double dist = 0;
    Edge<T> *path = nullptr;
    std::vector<Edge<T> *> incoming;
    int queueIndex = 0;

    int  webId   = -1;
    int  color   = -1;
    bool disabled = false;
    bool spilled  = false;

    std::vector<std::string> varNames;
    std::vector<int>         programPoints;
    std::vector<int>         defPoints;
    std::vector<int>         usePoints;

    void deleteEdge(Edge<T> *edge);
};

#endif // DA_GRAPH_VERTEX_DECL_HPP

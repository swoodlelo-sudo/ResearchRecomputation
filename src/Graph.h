// This file has the function declarations for loading, saving, adding edges,
// and accessing the graph. The function definitions are in Graph.cpp.
// n and m come from the sizes of the adjacency vector and edge list.

// This is the graph used by Dijkstra, DDFLP-D without deletion, and DDFLP-D-WL. I also use it with 
// the BFS cut tool to get the portions of data I need. As well with creating the edge struct (u, v, weight), 
// an adjacency entry with two fields being neighbor and edge ID, and the graph class with the edge list,
// vectors of adjacency entries and non pos weight.

// To put it simply this is for our undirected weighted graph that is stored
// as adjacency lists with one fixed source s.

#ifndef GRAPH_H
#define GRAPH_H

#include <limits>
#include <string>
#include <vector>

// 64 bit so repeated doubling has room
struct Edge {
    int u;
    int v;
    long long weight;
};

// The weight of the edge is kept in the edge list not in this entry.
struct Adjacent {
    int neighbor;
    int edgeId;
};

// An undirected edge shows up in two adjacency lists. If each list kept
// its own copy of the weight a batch would have to change both copies,
// and they could get out of sync. So the weight is kept in one place
// the edge list. Both adjacency lists point to it by edge ID.
//
// File format (our own, simple, with 0-based vertex IDs):
//     first line:   n m s
//     then m lines: u v weight
// Each undirected edge is listed once in the file.
class Graph {
private:
    int source;
    std::vector<Edge> edges;
    std::vector<std::vector<Adjacent>> adjacency;

public:
    // Our infinity value for distances.
    static constexpr long long INF_DISTANCE =
        std::numeric_limits<long long>::max();

    Graph();
    Graph(int vertexCount, int sourceVertex);

    void reset(int vertexCount, int sourceVertex);

    // Adds one undirected edge and returns its ID.
    // The caller must supply valid vertex IDs. load() checks file inputs first.
    int addEdge(int u, int v, long long weight);

    [[nodiscard]] int getVertexCount() const;
    [[nodiscard]] int getEdgeCount() const;
    [[nodiscard]] int getSource() const;

    [[nodiscard]] const Edge& getEdge(int edgeId) const;
    [[nodiscard]] long long getWeight(int edgeId) const;
    void setWeight(int edgeId, long long weight);

    // The endpoint of an edge that is not the given vertex.
    // The given vertex must belong to this edge.
    [[nodiscard]] int otherEndpoint(int edgeId, int vertex) const;

    [[nodiscard]] const std::vector<Adjacent>& neighborsOf(int vertex) const;
    [[nodiscard]] int degreeOf(int vertex) const;

    // Returns false for an unreadable file, invalid header, incomplete edge,
    // invalid endpoint, or non-positive weight. A failed load can leave a
    // partially loaded graph so only use it when this returns true.
    [[nodiscard]] bool load(const std::string& fileName);
    [[nodiscard]] bool save(const std::string& fileName) const;
};

#endif
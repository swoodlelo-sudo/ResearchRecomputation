#ifndef DDFLPD_WL_DIJKSTRA_H
#define DDFLPD_WL_DIJKSTRA_H

#include "Graph.h"
#include "MinHeap.h"

#include <vector>

// Algorithm 4 of our pseudo, DIJKSTRA-COUNT: binary heap Dijkstra with
// an optional work count.
//
// EXPLANATION:
// It is used in three places:
//     1. the first run that builds the starting tree (gives W_D)
//     2. the Dijkstra recomputation method, after every batch
//     3. the fallback of DDFLP-D-WL
//
// When counting is on, the work count W goes up by 1 for:
//     each queue insert
//     each extract-min
//     each edge look
//     each decrease-key
//
// When every vertex is reachable with a finite distance below INF_DISTANCE,
// W = 1 + n + 2m + I_D for our undirected graph.
// I_D is the number of successful relaxations after the source is inserted:
// each one causes either an insert or a decrease-key.
// If the graph is disconnected, only reached vertices and their edge looks
// contribute to W.
//
// The D labels in the implementation are the line labels of the pseudo.
// On ties the first parent that reached the minimum is kept.
class Dijkstra {
public:
    // Before calling, set up Q for the graph's vertex count. Q must be empty.
    // Reuse that setup for later runs on the same vertex IDs.
    // The graph must have a valid source and positive weights. Finite shortest
    // distances must be smaller than Graph::INF_DISTANCE.
    // Fills distance and parent (-1 for none), and leaves Q empty.
    // Returns W when counting is true, or 0 when counting is false.
    [[nodiscard]] static unsigned long long run(
        const Graph& graph,
        MinHeap& Q,
        std::vector<long long>& distance,
        std::vector<int>& parent,
        bool counting);
};

#endif
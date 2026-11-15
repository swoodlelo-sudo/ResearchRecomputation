#ifndef DDFLPD_WL_DIJKSTRA_H
#define DDFLPD_WL_DIJKSTRA_H

#include "Graph.h"
#include "MinHeap.h"
#include "Status.h"

#include <vector>

// Algorithm 4 of our pseudo, DIJKSTRA-COUNT: binary heap Dijkstra with
// an optional work count.

// The first run that builds the starting tree (gives W_D), then Dijkstra recomputation method, after every batch
//  at the end fallback of DDFLP-D-WL

// When counting is on, the work count W goes up by 1 for each queue insert, extract-min, edge look and each decrease-key

// When every vertex is reachable with a finite distance below INF_DISTANCE,
// W = 1 + n + 2m + I_D for our undirected graph. I_D is the number of successful relaxations after the source is inserted
// each one causes either an insert or a decrease-key.
// If the graph is disconnected only reached vertices and their edge lookscontribute to W.

// The D labels in the implementation are the line labels of the pseudo.
// On ties the first parent that reached the minimum is kept.

class Dijkstra
{
public:
    // Before calling, set up Q for the graph's vertex count. Q must be empty.
    // Reuse that setup for later runs on the same vertex IDs.
    // The graph must have a valid source and positive weights. A candidate
    // distance at or above Graph::INF_DISTANCE stops the run with Overflow.
    // On Ok, fills distance and parent (-1 for none).
    // Returns Ok or Overflow; W is written to work (0 when counting is off).
    // On Overflow, distance and parent are incomplete and must not be used.
    // Q is left empty on either status.
    [[nodiscard]] static Status run(
        const Graph &graph,
        MinHeap &Q,
        std::vector<long long> &distance,
        std::vector<int> &parent,
        unsigned long long &work,
        bool counting);
};

#endif
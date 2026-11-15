// This file has the Dijkstra function declared in Dijkstra.h.
// The caller owns Q so the repair code and fallback can reuse the same heap.

#include "Dijkstra.h"

#include <cassert>

unsigned long long Dijkstra::run(
    const Graph& graph,
    MinHeap& Q,
    std::vector<long long>& distance,
    std::vector<int>& parent,
    const bool counting) {

    const int vertexCount = graph.getVertexCount();
    const int source = graph.getSource();

    assert(source >= 0 && source < vertexCount);
    assert(Q.empty());

    unsigned long long work = 0;                         // D1

    distance.assign(vertexCount, Graph::INF_DISTANCE);   // D2
    parent.assign(vertexCount, -1);

    distance[source] = 0;                                // D3
    Q.insert(source, 0);
    if (counting) {
        work += 1;
    }

    while (!Q.empty()) {                                 // D4
        const int v = Q.extractMin(); // d(v) is now final.
        if (counting) {
            work += 1;
        }

        for (const Adjacent& adjacent : graph.neighborsOf(v)) { // D5
            // Count the edge look before any if that could skip this edge.
            if (counting) {
                work += 1;
            }

            const int x = adjacent.neighbor;
            const long long weight = graph.getWeight(adjacent.edgeId);

            // Avoid overflowing long long or using the infinity value as
            // a finite candidate distance. The edge look is already counted.
            if (weight >= Graph::INF_DISTANCE - distance[v]) {
                continue;
            }

            const long long candidate = distance[v] + weight;

            if (candidate < distance[x]) {               // D6
                distance[x] = candidate;
                parent[x] = v;

                if (Q.contains(x)) {
                    Q.decreaseKey(x, candidate);
                } else {
                    Q.insert(x, candidate);
                }

                // One queue operation: either insert or decrease-key.
                if (counting) {
                    work += 1;
                }
            }
        }
    }

    return work;                                        // D7
}
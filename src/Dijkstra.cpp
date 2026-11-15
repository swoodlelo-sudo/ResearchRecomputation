// This file has the Dijkstra function declared in Dijkstra.h.
// The caller owns Q so the repair code and fallback can reuse the same heap.

#include "Dijkstra.h"

#include <cassert>

Status Dijkstra::run(
    const Graph &graph,
    MinHeap &Q,
    std::vector<long long> &distance,
    std::vector<int> &parent,
    unsigned long long &work,
    const bool counting)
{

    work = 0; // D1

    const int vertexCount = graph.getVertexCount();
    const int source = graph.getSource();

    assert(source >= 0 && source < vertexCount);
    assert(Q.empty());

    distance.assign(vertexCount, Graph::INF_DISTANCE); // D2
    parent.assign(vertexCount, -1);

    distance[source] = 0; // D3
    Q.insert(source, 0);
    if (counting)
    {
        if (!checkedIncrement(work))
        {
            Q.clear();
            return Status::Overflow;
        }
    }

    while (!Q.empty())
    {                                 // D4
        const int v = Q.extractMin(); // d(v) is now final.
        if (counting)
        {
            if (!checkedIncrement(work))
            {
                Q.clear();
                return Status::Overflow;
            }
        }

        for (const Adjacent &adjacent : graph.neighborsOf(v))
        { // D5
            // Count the edge look before any if that could skip this edge.
            if (counting)
            {
                if (!checkedIncrement(work))
                {
                    Q.clear();
                    return Status::Overflow;
                }
            }

            const int x = adjacent.neighbor;
            const long long weight = graph.getWeight(adjacent.edgeId);

            // Stop if adding would overflow or reach the infinity sentinel.
            // The edge look is already counted. Empty Q before returning.
            if (weight >= Graph::INF_DISTANCE - distance[v])
            {
                Q.clear();
                return Status::Overflow;
            }

            const long long candidate = distance[v] + weight;

            if (candidate < distance[x])
            { // D6
                distance[x] = candidate;
                parent[x] = v;

                if (Q.contains(x))
                {
                    Q.decreaseKey(x, candidate);
                }
                else
                {
                    Q.insert(x, candidate);
                }

                // One queue operation: either insert or decrease-key.
                if (counting)
                {
                    if (!checkedIncrement(work))
                    {
                        Q.clear();
                        return Status::Overflow;
                    }
                }
            }
        }
    }

    return Status::Ok; // D7
}
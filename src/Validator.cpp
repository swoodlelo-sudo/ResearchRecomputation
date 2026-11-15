// This file has the validator function declared in Validator.h.

#include "Validator.h"
#include "Dijkstra.h"

#include <cstddef>
#include <iostream>

bool Validator::validateOutput(
    const Graph &graph,
    const std::vector<long long> &distance,
    const std::vector<int> &parent,
    MinHeap &referenceQ)
{

    const int vertexCount = graph.getVertexCount();
    const int source = graph.getSource();

    // Check the sizes before using vertex IDs to index either vector.
    if (distance.size() != static_cast<std::size_t>(vertexCount))
    {
        std::cerr << "Validation error: expected " << vertexCount
                  << " distance entries, got " << distance.size() << ".\n";
        return false;
    }
    if (parent.size() != static_cast<std::size_t>(vertexCount))
    {
        std::cerr << "Validation error: expected " << vertexCount
                  << " parent entries, got " << parent.size() << ".\n";
        return false;
    }

    // 1. Fresh reference run, with its own output vectors and heap.
    std::vector<long long> refDist;
    std::vector<int> refParent;
    unsigned long long referenceWork = 0;

    const Status status =
        Dijkstra::run(graph, referenceQ, refDist, refParent,
                      referenceWork, false);

    if (status != Status::Ok)
    {
        std::cerr << "Validation error: reference Dijkstra overflowed.\n";
        return false;
    }

    // 2. Exact distances. This also checks that the source distance is 0.
    for (int v = 0; v < vertexCount; v++)
    {
        if (distance[v] != refDist[v])
        {
            std::cerr << "Validation error at vertex " << v
                      << ": expected distance " << refDist[v]
                      << ", got " << distance[v] << ".\n";
            return false;
        }
    }

    // 3. The source must have no parent.
    if (parent[source] != -1)
    {
        std::cerr << "Validation error at vertex " << source
                  << ": expected source parent -1, got "
                  << parent[source] << ".\n";
        return false;
    }

    // 4. Check the parent of every other reached vertex.
    for (int v = 0; v < vertexCount; v++)
    {
        if (v == source || refDist[v] == Graph::INF_DISTANCE)
        {
            continue;
        }

        const int p = parent[v];
        if (p < 0 || p >= vertexCount)
        {
            std::cerr << "Validation error at vertex " << v
                      << ": expected parent ID from 0 to " << vertexCount - 1
                      << ", got " << p << ".\n";
            return false;
        }

        // This is the tight-edge equation rearranged to avoid addition
        // overflow. The distances have already passed the reference check.
        const long long requiredWeight = distance[v] - distance[p];
        bool neighborFound = false;
        bool tight = false;
        long long firstWeight = 0;

        for (const Adjacent &adjacent : graph.neighborsOf(v))
        {
            if (adjacent.neighbor != p)
            {
                continue;
            }

            const long long weight = graph.getWeight(adjacent.edgeId);
            if (!neighborFound)
            {
                firstWeight = weight;
            }
            neighborFound = true;

            // If there are parallel edges, any tight edge to p is valid.
            if (weight == requiredWeight)
            {
                tight = true;
                break;
            }
        }

        if (!neighborFound)
        {
            std::cerr << "Validation error at vertex " << v
                      << ": expected an edge to parent " << p
                      << ", got no such edge.\n";
            return false;
        }
        if (!tight)
        {
            std::cerr << "Validation error at vertex " << v
                      << ": expected an edge to parent " << p
                      << " with weight " << requiredWeight
                      << ", got no matching weight (first edge weight "
                      << firstWeight << ").\n";
            return false;
        }
    }

    // 5. Unreached vertices must have no parent.
    for (int v = 0; v < vertexCount; v++)
    {
        if (refDist[v] == Graph::INF_DISTANCE && parent[v] != -1)
        {
            std::cerr << "Validation error at vertex " << v
                      << ": expected parent -1 for an unreached vertex, got "
                      << parent[v] << ".\n";
            return false;
        }
    }

    return true;
}
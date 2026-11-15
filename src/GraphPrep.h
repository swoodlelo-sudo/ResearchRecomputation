#ifndef GRAPH_PREP_H
#define GRAPH_PREP_H

#include "Graph.h"

#include <string>
#include <vector>

// Counts from conversion and BFS. Loops are counted as arcs, but are not
// counted again as merged duplicates. The caller fills componentSize after BFS.
struct PrepStats {
    long long arcLines = 0;
    long long selfLoops = 0;
    long long duplicatesMerged = 0;
    long long edges = 0;
    long long componentSize = 0;
};

// This prepares the graphs all three methods run on. It converts the DIMACS
// arcs into undirected edges, then cuts connected prefixes of the BFS order.
// Full vertex indices are original DIMACS IDs minus 1.
class GraphPrep {
public:
    // Keep the smallest weight for each pair and add edges in original-ID
    // order. Returns false for a bad header, arc, tag, or arc count.
    // Only use full and stats when conversion succeeds.
    static bool convertDimacs(const std::string& fileName, Graph& full,
                              PrepStats& stats);

    // Uses order as the queue. On the converted full graph, neighbors are
    // visited in ascending original ID. order.size() is the component size.
    static void bfsOrder(const Graph& graph, int source,
                         std::vector<int>& order);

    static bool neighborsAscending(const Graph& graph);

    // order must be the full graph's BFS order, starting at its source.
    // Local IDs are positions in that order. Keep full-edge relative order,
    // including when the local endpoint numbers are no longer ascending.
    static bool cutGraph(const Graph& full, const std::vector<int>& order,
                         int N, Graph& cut, std::vector<int>& localToFull);

    // First line N, then localId originalId. The output folder must exist.
    static bool saveMapping(const std::vector<int>& localToFull,
                            const std::string& fileName);
};

#endif
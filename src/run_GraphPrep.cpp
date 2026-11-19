// Convert the Northwest file, cut the five BFS prefixes, and check each saved
// graph with Dijkstra. Run from the project root so the data paths match.

#include "GraphPrep.h"
#include "Dijkstra.h"
#include "MinHeap.h"

#include <filesystem>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

int main() {
    Graph full;
    PrepStats stats;

    if (!GraphPrep::convertDimacs("data/raw/USA-road-t.NE.gr", full, stats)) {
        std::cerr << "Could not convert the DIMACS file.\n";
        return 1;
    }

    std::cout << "Arc lines: " << stats.arcLines << '\n'
              << "Self-loops: " << stats.selfLoops << '\n'
              << "Merged duplicates: " << stats.duplicatesMerged << '\n'
              << "Unique undirected edges: " << stats.edges << '\n';

    if (!GraphPrep::neighborsAscending(full)) {
        std::cerr << "Neighbor lists are not strictly ascending.\n";
        return 1;
    }

    std::vector<int> order;
    GraphPrep::bfsOrder(full, 0, order);

    stats.componentSize = static_cast<long long>(order.size());
    std::cout << "Source component size: " << stats.componentSize << '\n';

    // Check the full component before creating any of the cut files.
    if (stats.componentSize < 810000) {
        std::cerr << "Source component has fewer than 810000 vertices.\n";
        return 1;
    }

    std::error_code error;
    std::filesystem::create_directories("data/graphs", error);

    if (error) {
        std::cerr << "Could not create data/graphs: " << error.message() << '\n';
        return 1;
    }

    const int sizes[] = {10000, 30000, 90000, 270000, 810000};

    for (int N : sizes) {
        Graph cut;
        std::vector<int> mapping;

        if (!GraphPrep::cutGraph(full, order, N, cut, mapping)) {
            std::cerr << "Failed to make cut " << N << '\n';
            return 1;
        }

        const std::string stem = "data/graphs/NE_" + std::to_string(N);

        if (!cut.save(stem + ".txt") ||
            !GraphPrep::saveMapping(mapping, stem + "_map.txt")) {
            std::cerr << "Failed to save cut " << N << '\n';
            return 1;
        }

        Graph loaded;

        if (!loaded.load(stem + ".txt")) {
            std::cerr << "Failed to load saved graph " << N << '\n';
            return 1;
        }

        MinHeap queue;
        std::vector<long long> distance;
        std::vector<int> parent;
        unsigned long long ignoredWork = 0;

        queue.setup(loaded.getVertexCount());

        if (Dijkstra::run(loaded, queue, distance, parent, ignoredWork, false)
            != Status::Ok) {
            std::cerr << "Dijkstra failed on cut " << N << '\n';
            return 1;
        }

        for (long long d : distance) {
            if (d == Graph::INF_DISTANCE) {
                std::cerr << "Unreachable vertex in cut " << N << '\n';
                return 1;
            }
        }

        std::cout << "N=" << loaded.getVertexCount()
                  << " m=" << loaded.getEdgeCount() << '\n';
    }

    return 0;
}
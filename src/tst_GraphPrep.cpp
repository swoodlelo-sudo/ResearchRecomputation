// Tests conversion, BFS numbering, induced cuts, mappings and saved graphs.

#include "GraphPrep.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>
#include <vector>

namespace {
bool report(bool passed, int number) {
    std::cout << (passed ? "PASS " : "FAIL ") << number << '\n';
    return passed;
}

bool sameGraph(const Graph& a, const Graph& b) {
    if (a.getVertexCount() != b.getVertexCount() ||
        a.getEdgeCount() != b.getEdgeCount() ||
        a.getSource() != b.getSource()) {
        return false;
    }

    for (int e = 0; e < a.getEdgeCount(); ++e) {
        const Edge& x = a.getEdge(e);
        const Edge& y = b.getEdge(e);

        if (x.u != y.u || x.v != y.v || x.weight != y.weight) return false;
    }

    return true;
}

bool savedGraphMatches(const Graph& graph, const Graph& expected,
                       const std::string& path) {
    if (!graph.save(path)) return false;

    Graph loaded;
    return loaded.load(path) && sameGraph(loaded, expected);
}

// Read back the mapping, too: a successful save alone does not show that
// local IDs and the 1-based original IDs were written correctly.
bool savedMappingMatches(const std::vector<int>& mapping,
                         const std::vector<int>& expected,
                         const std::string& path) {
    if (mapping != expected || !GraphPrep::saveMapping(mapping, path)) {
        return false;
    }

    std::ifstream in(path);
    int n = 0;

    if (!(in >> n) || n != static_cast<int>(expected.size())) return false;

    for (int i = 0; i < n; ++i) {
        int localId = -1;
        int originalId = -1;

        if (!(in >> localId >> originalId) || localId != i ||
            originalId != expected[i] + 1) {
            return false;
        }
    }

    in >> std::ws;
    return in.eof() && !in.bad();
}
}

int main() {
    std::error_code error;
    std::filesystem::create_directories("build", error);

    if (error) {
        std::cerr << "FAIL: could not create build: " << error.message() << '\n';
        return 1;
    }

    Graph full;
    PrepStats stats;

    if (!report(
            GraphPrep::convertDimacs("data/test/prep_small.gr", full, stats) &&
            stats.arcLines == 10 && stats.selfLoops == 1 &&
            stats.duplicatesMerged == 5 && stats.edges == 4, 1)) {
        return 1;
    }

    Graph expectedFull(6, 0);
    expectedFull.addEdge(0, 1, 5);
    expectedFull.addEdge(0, 3, 3);
    expectedFull.addEdge(1, 2, 2);
    expectedFull.addEdge(3, 4, 1);

    if (!report(sameGraph(full, expectedFull), 2)) return 1;
    if (!report(GraphPrep::neighborsAscending(full), 3)) return 1;

    std::vector<int> order;
    GraphPrep::bfsOrder(full, 0, order);
    stats.componentSize = static_cast<long long>(order.size());

    if (!report(order == std::vector<int>({0, 1, 3, 2, 4}) &&
                stats.componentSize == 5, 4)) {
        return 1;
    }

    Graph cut;
    std::vector<int> mapping;

    Graph expected3(3, 0);
    expected3.addEdge(0, 1, 5);
    expected3.addEdge(0, 2, 3);

    if (!report(
            GraphPrep::cutGraph(full, order, 3, cut, mapping) &&
            savedGraphMatches(cut, expected3, "build/prep_cut3.txt") &&
            savedMappingMatches(mapping, {0, 1, 3},
                                "build/prep_cut3_map.txt"), 5)) {
        return 1;
    }

    Graph expected5(5, 0);
    expected5.addEdge(0, 1, 5);
    expected5.addEdge(0, 2, 3);
    expected5.addEdge(1, 3, 2);
    expected5.addEdge(2, 4, 1);

    if (!report(
            GraphPrep::cutGraph(full, order, 5, cut, mapping) &&
            savedGraphMatches(cut, expected5, "build/prep_cut5.txt") &&
            savedMappingMatches(mapping, {0, 1, 3, 2, 4},
                                "build/prep_cut5_map.txt"), 6)) {
        return 1;
    }

    if (!report(!GraphPrep::cutGraph(full, order, 6, cut, mapping), 7)) {
        return 1;
    }

    Graph bad;
    PrepStats badStats;

    if (!report(
            !GraphPrep::convertDimacs("data/test/prep_bad.gr", bad, badStats),
            8)) {
        return 1;
    }

    Graph fig;
    PrepStats figStats;

    if (!GraphPrep::convertDimacs("data/test/fig1.gr", fig, figStats)) {
        report(false, 9);
        return 1;
    }

    GraphPrep::bfsOrder(fig, 0, order);

    Graph expectedFig;

    if (!report(
            GraphPrep::cutGraph(fig, order, 4, cut, mapping) &&
            expectedFig.load("data/test/fig1.txt") &&
            savedGraphMatches(cut, expectedFig, "build/prep_fig1.txt"), 9)) {
        return 1;
    }

    std::cout << "All 9 GraphPrep cases passed.\n";
    return 0;
}
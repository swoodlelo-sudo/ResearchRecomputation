// Test both graphs with counting on and off before moving to the next step.

#include "Dijkstra.h"

#include <iostream>
#include <string>
#include <vector>

bool testGraph(const std::string& fileName,
               const std::vector<long long>& expectedDistance,
               const std::vector<int>& expectedParent,
               const unsigned long long expectedWork) {
    Graph graph;
    if (!graph.load(fileName)) {
        std::cerr << "FAIL: could not load " << fileName << "\n";
        return false;
    }

    // Set up Q once for this graph, then reuse it for both runs.
    MinHeap Q;
    Q.setup(graph.getVertexCount());

    std::vector<long long> distance;
    std::vector<int> parent;

    const unsigned long long work =
        Dijkstra::run(graph, Q, distance, parent, true);

    std::cout << fileName << "\ndist: ";
    for (const long long value : distance) {
        std::cout << value << " ";
    }

    std::cout << "\nparent: ";
    for (const int value : parent) {
        std::cout << value << " ";
    }
    std::cout << "\nW = " << work << "\n";

    if (distance != expectedDistance || parent != expectedParent ||
        work != expectedWork || !Q.empty()) {
        std::cerr << "FAIL: counted result does not match for "
                  << fileName << "\n";
        return false;
    }

    // The first run emptied Q, so no new setup or clear is needed here.
    const unsigned long long uncountedWork =
        Dijkstra::run(graph, Q, distance, parent, false);

    if (distance != expectedDistance || parent != expectedParent ||
        uncountedWork != 0 || !Q.empty()) {
        std::cerr << "FAIL: uncounted result does not match for "
                  << fileName << "\n";
        return false;
    }

    std::cout << "PASS: counting on and off\n\n";
    return true;
}

int main() {
    const bool fig1Passed = testGraph(
        "data/test/fig1.txt", {0, 1, 2, 3}, {-1, 0, 0, 1}, 16ULL);

    const bool fig2Passed = testGraph(
        "data/test/fig2.txt", {0, 1, 2}, {-1, 0, 1}, 13ULL);

    if (!fig1Passed || !fig2Passed) {
        return 1;
    }

    std::cout << "All Dijkstra tests passed.\n";
    return 0;
}
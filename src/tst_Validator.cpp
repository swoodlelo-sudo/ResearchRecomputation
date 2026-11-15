// A row passes when the validator returns the expected answer.
// Deliberately wrong results should be rejected by the validator.

#include "Validator.h"
#include "Dijkstra.h"

#include <iostream>
#include <vector>

bool checkCase(const int number,
               const char* description,
               const Graph& graph,
               const std::vector<long long>& distance,
               const std::vector<int>& parent,
               MinHeap& referenceQ,
               const bool expected) {
    const bool actual =
        Validator::validateOutput(graph, distance, parent, referenceQ);

    const bool passed = actual == expected;
    std::cout << (passed ? "PASS" : "FAIL") << " " << number << ": "
              << description << " (expected " << expected
              << ", got " << actual << ")\n";
    return passed;
}

int main() {
    Graph fig1;
    Graph fig3;
    if (!fig1.load("data/test/fig1.txt") ||
        !fig3.load("data/test/fig3.txt")) {
        std::cerr << "FAIL: could not load the test graphs.\n";
        return 1;
    }

    // Both test graphs have four vertices. Each queue owns its own state.
    MinHeap methodQ;
    MinHeap referenceQ;
    methodQ.setup(4);
    referenceQ.setup(4);

    std::vector<long long> distance;
    std::vector<int> parent;
    (void)Dijkstra::run(fig1, methodQ, distance, parent, false);

    const std::vector<long long> originalDistance = distance;
    const std::vector<int> originalParent = parent;
    int failures = 0;
    std::cout << std::boolalpha;

    // 1. The original Dijkstra result should pass.
    if (!checkCase(1, "original fig1 result", fig1, distance, parent,
                   referenceQ, true)) {
        failures++;
    }

    // 2. Change C's distance from 3 to 4.
    distance[3] = 4;
    if (!checkCase(2, "wrong distance rejected", fig1, distance, parent,
                   referenceQ, false)) {
        failures++;
    }

    // 3. B is C's neighbor, but 2 + 4 = 6 does not equal C's distance of 3.
    distance = originalDistance;
    parent[3] = 2;
    if (!checkCase(3, "non-tight parent rejected", fig1, distance, parent,
                   referenceQ, false)) {
        failures++;
    }

    // 4. B is not A's neighbor.
    parent = originalParent;
    parent[1] = 2;
    if (!checkCase(4, "non-neighbor parent rejected", fig1, distance, parent,
                   referenceQ, false)) {
        failures++;
    }

    // 5. In fig3, both A and B give C the same distance of 2.
    (void)Dijkstra::run(fig3, methodQ, distance, parent, false);
    parent[3] = 2;
    if (!checkCase(5, "tied parent allowed", fig3, distance, parent,
                   referenceQ, true)) {
        failures++;
    }

    // 6. In fig1.txt, edge 0 is S-A (0-1). Keep the old output after updating it.
    distance = originalDistance;
    parent = originalParent;
    fig1.setWeight(0, 5);
    if (!checkCase(6, "stale result rejected", fig1, distance, parent,
                   referenceQ, false)) {
        failures++;
    }

    // 7. Recompute on the updated graph, then check the exact expected output.
    (void)Dijkstra::run(fig1, methodQ, distance, parent, false);
    const bool accepted =
        Validator::validateOutput(fig1, distance, parent, referenceQ);

    const std::vector<long long> updatedDistance = {0, 5, 2, 6};
    const std::vector<int> updatedParent = {-1, 0, 0, 2};
    const bool passed = accepted && distance == updatedDistance &&
                        parent == updatedParent;

    std::cout << (passed ? "PASS" : "FAIL")
              << " 7: recomputed fig1 result and expected numbers\n";
    if (!passed) {
        failures++;
    }

    return failures == 0 ? 0 : 1;
}
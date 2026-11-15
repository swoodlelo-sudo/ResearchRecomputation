// Tests exact limits, counted repair, switching and frozen calibration.

#include "DDFLPD_WL.h"
#include "Validator.h"

#include <iostream>
#include <limits>
#include <vector>

namespace {
int failures = 0;

void report(int number, bool passed) {
    std::cout << (passed ? "PASS " : "FAIL ") << number << '\n';
    if (!passed) ++failures;
}

Batch makeBatch(int edgeId, long long oldWeight, long long newWeight) {
    return Batch{0, 1, {{edgeId, oldWeight, newWeight}}};
}

bool limitEquals(unsigned long long S, int num, int den,
                 unsigned long long expected) {
    unsigned long long limit = 0;

    return DDFLPD_WL::computeLimit(S, num, den, limit) &&
           limit == expected;
}

bool frozen(const DDFLPD_WL& method, unsigned long long expectedL) {
    return method.getL() == expectedL &&
           method.getWD() == 16 &&
           method.getWB() == 8;
}

bool checkTree(const Graph& graph, const DDFLPD_WL& method,
               const std::vector<long long>& expectedDistance,
               const std::vector<int>& expectedParent) {
    const DDFLPD& core = method.repairCore();

    MinHeap referenceQ;
    referenceQ.setup(graph.getVertexCount());

    const bool valid = Validator::validateOutput(
        graph, core.distances(), core.parents(), referenceQ);

    const bool support = core.checkSupport(graph);

    return valid && support &&
           core.distances() == expectedDistance &&
           core.parents() == expectedParent;
}

// Cases 3-7 use the same graph and batch, with different alpha values.
bool oneBatch(int num, int den, bool infinite, unsigned long long expectedL,
              bool expectedSwitch, unsigned long long expectedWR,
              unsigned long long expectedRestart) {
    Graph graph;
    if (!graph.load("data/test/fig1.txt")) return false;

    DDFLPD_WL method(num, den, infinite);

    if (method.initialize(graph) != Status::Ok ||
        !frozen(method, expectedL)) {
        return false;
    }

    const WLOutcome outcome = method.update(graph, makeBatch(0, 1, 5));
    if (outcome.status != Status::Ok) return false;

    const bool tree = checkTree(
        graph, method, {0, 5, 2, 6}, {-1, 0, 0, 2});

    return tree && frozen(method, expectedL) &&
           outcome.switched == expectedSwitch &&
           outcome.wR == expectedWR &&
           outcome.wPreSwitch == (expectedSwitch ? expectedWR : 0) &&
           outcome.wRestart == expectedRestart;
}

bool twoBatches() {
    Graph graph;
    if (!graph.load("data/test/fig1.txt")) return false;

    DDFLPD_WL method(1, 4, false);

    if (method.initialize(graph) != Status::Ok || !frozen(method, 6)) {
        return false;
    }

    // First fallback: repair 7, restart 24, total 31.
    const WLOutcome first = method.update(graph, makeBatch(0, 1, 5));
    if (first.status != Status::Ok) return false;

    const bool firstTree = checkTree(
        graph, method, {0, 5, 2, 6}, {-1, 0, 0, 2});

    if (!firstTree || !frozen(method, 6) || !first.switched ||
        first.wR != 7 || first.wPreSwitch != 7 || first.wRestart != 24 ||
        first.wR + first.wRestart != 31) {
        return false;
    }

    // B-C is now a tree edge. This rebuild takes Dijkstra 17 + support 8.
    const WLOutcome second = method.update(graph, makeBatch(3, 4, 8));
    if (second.status != Status::Ok) return false;

    const bool secondTree = checkTree(
        graph, method, {0, 5, 2, 7}, {-1, 0, 0, 1});

    return secondTree && frozen(method, 6) && second.switched &&
           second.wR == 7 && second.wPreSwitch == 7 &&
           second.wRestart == 25 &&
           second.wR + second.wRestart == 32;
}
}

int main() {
    // 1: S = 24.
    report(1, limitEquals(24, 4, 4, 24) &&
              limitEquals(24, 3, 4, 18) &&
              limitEquals(24, 2, 4, 12) &&
              limitEquals(24, 1, 4, 6));

    // 2: S = 25 checks that division rounds down.
    report(2, limitEquals(25, 3, 4, 18) &&
              limitEquals(25, 2, 4, 12) &&
              limitEquals(25, 1, 4, 6));

    // 3: alpha = 1. Repair finishes at 17.
    report(3, oneBatch(1, 1, false, 24, false, 17, 0));

    // 4: alpha = 0.75. Repair finishes at 17.
    report(4, oneBatch(3, 4, false, 18, false, 17, 0));

    // 5: alpha = 0.5. Repair 13 + restart 24 = 37.
    report(5, oneBatch(1, 2, false, 12, true, 13, 24));

    // 6: alpha = 0.25. Repair 7 + restart 24 = 31.
    report(6, oneBatch(1, 4, false, 6, true, 7, 24));

    // 7: infinity still counts, but never switches.
    report(7, oneBatch(
        0, 1, true,
        std::numeric_limits<unsigned long long>::max(),
        false, 17, 0));

    // 8: two fallbacks on one object; L, W_D and W_B stay frozen.
    report(8, twoBatches());

    if (failures != 0) return 1;

    std::cout << "All 8 DDFLPD_WL cases passed.\n";
    return 0;
}
// Test both normal graphs, distance overflow, and the checked work increment.

#include "Dijkstra.h"

#include <iostream>
#include <limits>
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

    unsigned long long work = 99;
    const Status status =
        Dijkstra::run(graph, Q, distance, parent, work, true);

    if (status != Status::Ok) {
        std::cerr << "FAIL: expected Ok, got Overflow for "
                  << fileName << "\n";
        return false;
    }

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
    unsigned long long uncountedWork = 99;
    const Status uncountedStatus =
        Dijkstra::run(graph, Q, distance, parent, uncountedWork, false);

    if (uncountedStatus != Status::Ok) {
        std::cerr << "FAIL: uncounted Dijkstra overflowed for "
                  << fileName << "\n";
        return false;
    }

    if (distance != expectedDistance || parent != expectedParent ||
        uncountedWork != 0 || !Q.empty()) {
        std::cerr << "FAIL: uncounted result does not match for "
                  << fileName << "\n";
        return false;
    }

    std::cout << "PASS: counting on and off\n\n";
    return true;
}

bool testOverflow() {
    Graph graph;
    if (!graph.load("data/test/fig_ovrflw.txt")) {
        std::cerr << "FAIL: could not load fig_ovrflw.txt\n";
        return false;
    }

    MinHeap Q;
    Q.setup(graph.getVertexCount());

    std::vector<long long> distance;
    std::vector<int> parent;
    const bool modes[] = {true, false};

    for (const bool counting : modes) {
        unsigned long long work = 99;
        const Status status =
            Dijkstra::run(graph, Q, distance, parent, work, counting);

        // The output vectors are incomplete on Overflow, so do not use them.
        if (status != Status::Overflow || !Q.empty()) {
            std::cerr << "FAIL: expected Overflow with Q empty\n";
            return false;
        }

        if (!counting && work != 0) {
            std::cerr << "FAIL: counting off should leave work at 0\n";
            return false;
        }

        for (int v = 0; v < graph.getVertexCount(); v++) {
            if (Q.contains(v)) {
                std::cerr << "FAIL: overflow left a vertex marked in Q\n";
                return false;
            }
        }
    }

    std::cout << "PASS: distance overflow leaves Q empty, counting on and off\n";
    return true;
}

bool testCheckedIncrement() {
    const unsigned long long maximum =
        std::numeric_limits<unsigned long long>::max();

    unsigned long long work = maximum;
    if (checkedIncrement(work) || work != maximum) {
        std::cerr << "FAIL: increment at the maximum should fail unchanged\n";
        return false;
    }

    work = 5;
    if (!checkedIncrement(work) || work != 6) {
        std::cerr << "FAIL: increment from 5 should succeed and produce 6\n";
        return false;
    }

    // Reaching the maximum is allowed; going beyond it is not.
    work = maximum - 1;
    if (!checkedIncrement(work) || work != maximum) {
        std::cerr << "FAIL: increment to the maximum should succeed\n";
        return false;
    }

    std::cout << "PASS: checked increment at the limit and from 5 to 6\n";
    return true;
}

int main() {
    const bool fig1Passed = testGraph(
        "data/test/fig1.txt", {0, 1, 2, 3}, {-1, 0, 0, 1}, 16ULL);

    const bool fig2Passed = testGraph(
        "data/test/fig2.txt", {0, 1, 2}, {-1, 0, 1}, 13ULL);

    const bool overflowPassed = testOverflow();
    const bool incrementPassed = testCheckedIncrement();

    if (!fig1Passed || !fig2Passed || !overflowPassed || !incrementPassed) {
        return 1;
    }

    std::cout << "All Dijkstra tests passed.\n";
    return 0;
}
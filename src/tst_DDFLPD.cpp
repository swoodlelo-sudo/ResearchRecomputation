// Tests the initial tree, measured work, fallback, ties and repeated batches.

#include "DDFLPD.h"
#include "Validator.h"

#include <iostream>
#include <limits>
#include <vector>

namespace {
int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

Batch makeBatch(int edgeId, long long oldWeight, long long newWeight) {
    Batch batch;
    batch.batchId = 0;
    batch.code = 1;
    batch.updates.push_back({edgeId, oldWeight, newWeight});

    return batch;
}

bool loadAndInitialize(const char* path, Graph& graph, DDFLPD& solver) {
    if (!graph.load(path) || solver.initialize(graph) != Status::Ok) {
        std::cerr << "FAIL: could not load and initialize " << path << '\n';
        return false;
    }

    return true;
}

bool valid(const Graph& graph, const DDFLPD& solver) {
    MinHeap referenceQ;
    referenceQ.setup(graph.getVertexCount());

    return Validator::validateOutput(graph, solver.distances(),
                                     solver.parents(), referenceQ) &&
           solver.checkSupport(graph);
}

void reportCase(int number) {
    static int previousFailures = 0;

    std::cout << (failures == previousFailures ? "PASS " : "FAIL ")
              << number << '\n';

    previousFailures = failures;
}

void checkTree(const DDFLPD& solver, const std::vector<long long>& distance,
               const std::vector<int>& parent, const char* message) {
    expect(solver.distances() == distance && solver.parents() == parent,
           message);
}
}

int main() {
    const unsigned long long unlimited =
        std::numeric_limits<unsigned long long>::max();

    const std::vector<long long> initialD = {0, 1, 2, 3};
    const std::vector<int> initialP = {-1, 0, 0, 1};

    const std::vector<long long> updatedD = {0, 5, 2, 6};
    const std::vector<int> updatedP = {-1, 0, 0, 2};

    // 1: initial tree and deterministic owners.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig1.txt", graph, solver)) return 1;

        expect(solver.getWD() == 16 && solver.getWB() == 8,
               "1: WD=16, WB=8");

        checkTree(solver, initialD, initialP, "1: initial shortest-path tree");

        expect(solver.edgeOwner(0) == 0 && solver.edgeOwner(1) == 2 &&
               solver.edgeOwner(2) == 1 && solver.edgeOwner(3) == 3,
               "1: edge owners S, B, A, C");

        expect(valid(graph, solver), "1: validator and support");
        reportCase(1);
    }

    // 2: all 17 repair operations finish.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig1.txt", graph, solver)) return 1;

        expect(solver.repair(graph, makeBatch(0, 1, 5), true, unlimited) ==
               RepairResult::Finished, "2: finish");

        expect(solver.lastWR() == 17, "2: WR=17");
        checkTree(solver, updatedD, updatedP, "2: updated tree");

        expect(solver.lastAffected() == 2 && solver.lastRed() == 2 &&
               solver.lastDegreeSum(graph) == 4,
               "2: affected, red, degrees");

        expect(valid(graph, solver), "2: validator and support");
        reportCase(2);
    }

    // 3: counting off does the same repair, with WR=0.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig1.txt", graph, solver)) return 1;

        expect(solver.repair(graph, makeBatch(0, 1, 5), false, 0) ==
               RepairResult::Finished, "3: finish uncounted");

        expect(solver.lastWR() == 0, "3: WR=0");
        checkTree(solver, updatedD, updatedP, "3: updated tree");

        expect(valid(graph, solver), "3: validator and support");
        reportCase(3);
    }

    // 4: limit 6 -> WR=7, rebuild costs 24.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig1.txt", graph, solver)) return 1;

        expect(solver.repair(graph, makeBatch(0, 1, 5), true, 6) ==
               RepairResult::LimitExceeded, "4: limit exceeded");

        expect(solver.lastWR() == 7, "4: WR=7");

        solver.abandonRepair();

        unsigned long long W = 0;

        expect(solver.rebuild(graph, W) == Status::Ok && W == 24 &&
               solver.lastWR() + W == 31,
               "4: rebuild=24, total=31");

        checkTree(solver, updatedD, updatedP, "4: rebuilt tree");
        expect(valid(graph, solver), "4: validator and support");
        reportCase(4);
    }

    // 5: equal to the limit is allowed.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig1.txt", graph, solver)) return 1;

        expect(solver.repair(graph, makeBatch(0, 1, 5), true, 17) ==
               RepairResult::Finished, "5: equal limit finishes");

        expect(solver.lastWR() == 17, "5: WR=17");
        checkTree(solver, updatedD, updatedP, "5: updated tree");

        expect(valid(graph, solver), "5: validator and support");
        reportCase(5);
    }

    // 6: limit 16 stops exactly at 17.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig1.txt", graph, solver)) return 1;

        expect(solver.repair(graph, makeBatch(0, 1, 5), true, 16) ==
               RepairResult::LimitExceeded, "6: limit 16 exceeded");

        expect(solver.lastWR() == 17, "6: WR=17");

        solver.abandonRepair();

        unsigned long long W = 0;

        expect(solver.rebuild(graph, W) == Status::Ok && W == 24 &&
               solver.lastWR() + W == 41,
               "6: rebuild=24, total=41");

        checkTree(solver, updatedD, updatedP, "6: rebuilt tree");
        expect(valid(graph, solver), "6: validator and support");
        reportCase(6);
    }

    // 7: empty batch.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig1.txt", graph, solver)) return 1;

        Batch batch;
        batch.batchId = 0;
        batch.code = 1;

        expect(solver.repair(graph, batch, true, unlimited) ==
               RepairResult::Finished, "7: empty batch finishes");

        expect(solver.lastWR() == 0, "7: WR=0");
        checkTree(solver, initialD, initialP, "7: unchanged tree");

        expect(valid(graph, solver), "7: validator and support");
        reportCase(7);
    }

    // 8: non-tree increase does not trigger repairs.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig1.txt", graph, solver)) return 1;

        expect(solver.repair(graph, makeBatch(3, 4, 8), true, unlimited) ==
               RepairResult::Finished, "8: non-tree batch finishes");

        expect(solver.lastWR() == 0, "8: WR=0");
        checkTree(solver, initialD, initialP, "8: unchanged tree");

        expect(valid(graph, solver), "8: validator and support");
        reportCase(8);
    }

    // 9: C goes pink through B, while A stays red.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig3.txt", graph, solver)) return 1;

        expect(solver.repair(graph, makeBatch(0, 1, 3), true, unlimited) ==
               RepairResult::Finished, "9: tie case finishes");

        expect(solver.lastWR() == 11, "9: WR=11");

        checkTree(solver, {0, 3, 1, 2}, {-1, 0, 0, 2},
                  "9: tie-case tree");

        expect(solver.lastAffected() == 2 && solver.lastRed() == 1,
               "9: affected 2, red 1");

        expect(valid(graph, solver), "9: validator and support");
        reportCase(9);
    }

    // 10: preserve state across two batches on one object.
    {
        Graph graph;
        DDFLPD solver;

        if (!loadAndInitialize("data/test/fig1.txt", graph, solver)) return 1;

        expect(solver.repair(graph, makeBatch(0, 1, 5), true, unlimited) ==
               RepairResult::Finished, "10: first batch finishes");

        expect(valid(graph, solver), "10: first batch support");

        expect(solver.repair(graph, makeBatch(3, 4, 8), true, unlimited) ==
               RepairResult::Finished, "10: second batch finishes");

        expect(solver.lastWR() == 8 && solver.lastAffected() == 1,
               "10: WR=8, affected=1");

        checkTree(solver, {0, 5, 2, 7}, {-1, 0, 0, 1},
                  "10: second batch tree");

        expect(valid(graph, solver), "10: validator and support");
        reportCase(10);
    }

    if (failures != 0) {
        std::cerr << failures << " DDFLPD check(s) failed.\n";
        return 1;
    }

    std::cout << "All 10 DDFLPD cases passed.\n";
    return 0;
}
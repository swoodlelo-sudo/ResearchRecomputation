
#include "Replay.h"
#include "Dijkstra.h"
#include "Validator.h"

#include <iostream>
#include <vector>

int failures = 0;

// Print an error if a test fails.
void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

// Make a batch containing one weight increase.
Batch makeBatch(int edgeId, long long oldWeight,
                long long newWeight) {
    Batch batch;
    batch.batchId = 0;
    batch.code = 1;
    batch.updates.push_back({edgeId, oldWeight, newWeight});
    return batch;
}

// Check all six operation counts and the switch flag.
void checkCounts(const ReplayCounts& c,
                 unsigned long long apply,
                 unsigned long long queueSetup,
                 unsigned long long repair,
                 unsigned long long dijkstra,
                 unsigned long long restart,
                 unsigned long long total,
                 bool switched,
                 const char* message) {

    bool correct =
        c.status == Status::Ok &&
        c.apply == apply &&
        c.queueSetup == queueSetup &&
        c.repair == repair &&
        c.dijkstra == dijkstra &&
        c.restart == restart &&
        c.total == total &&
        c.switched == switched;

    expect(correct, message);

    if (!correct) {
        std::cerr << "  Got: apply=" << c.apply
                  << " queue=" << c.queueSetup
                  << " repair=" << c.repair
                  << " dijkstra=" << c.dijkstra
                  << " restart=" << c.restart
                  << " total=" << c.total
                  << " switched=" << c.switched << '\n';
    }
}

// Validate a Dijkstra result.
bool validDijkstra(const Graph& graph,
                   const std::vector<long long>& dist,
                   const std::vector<int>& parent) {

    MinHeap refQ;
    refQ.setup(graph.getVertexCount());

    return Validator::validateOutput(graph, dist, parent, refQ);
}

// Validate the baseline and its support structures.
bool validBaseline(const Graph& graph, const DDFLPD& core) {

    MinHeap refQ;
    refQ.setup(graph.getVertexCount());

    return Validator::validateOutput(
        graph, core.distances(), core.parents(), refQ
    ) && core.checkSupport(graph);
}

// Validate the wrapper and its repair core.
bool validWrapper(const Graph& graph, const DDFLPD_WL& wl) {

    MinHeap refQ;
    refQ.setup(graph.getVertexCount());

    const DDFLPD& core = wl.repairCore();

    return Validator::validateOutput(
        graph, core.distances(), core.parents(), refQ
    ) && core.checkSupport(graph);
}

int main() {

    // Case 1: Dijkstra, S-A increases from 1 to 5.
    {
        Graph graph;
        if (!graph.load("data/test/fig1.txt")) return 1;

        MinHeap Q;
        Q.setup(graph.getVertexCount());

        std::vector<long long> dist;
        std::vector<int> parent;
        unsigned long long W = 0;

        // Build the initial shortest-path tree.
        if (Dijkstra::run(
                graph, Q, dist, parent, W, false
            ) != Status::Ok) return 1;

        ReplayCounts c = Replay::dijkstraBatch(
            graph, makeBatch(0, 1, 5), Q, dist, parent
        );

        checkCounts(c, 0, 0, 0, 16, 0, 16, false,
                    "1: Dijkstra counts");

        expect(validDijkstra(graph, dist, parent),
               "1: Dijkstra validator");
    }

    // Case 2: Baseline, S-A increases from 1 to 5.
    {
        Graph graph;
        if (!graph.load("data/test/fig1.txt")) return 1;

        DDFLPD core;
        if (core.initialize(graph) != Status::Ok) return 1;

        ReplayCounts c = Replay::baselineBatch(
            graph, makeBatch(0, 1, 5), core
        );

        checkCounts(c, 1, 1, 17, 0, 0, 19, false,
                    "2: Baseline counts");

        expect(validBaseline(graph, core),
               "2: Baseline validator and support");
    }

    // Case 3: Wrapper alpha = 1, no switch.
    {
        Graph graph;
        if (!graph.load("data/test/fig1.txt")) return 1;

        DDFLPD_WL wl(1, 1, false);
        if (wl.initialize(graph) != Status::Ok) return 1;

        ReplayCounts c = Replay::wrapperBatch(
            graph, makeBatch(0, 1, 5), wl
        );

        checkCounts(c, 1, 1, 17, 0, 0, 19, false,
                    "3: Wrapper alpha 1 counts");

        expect(validWrapper(graph, wl),
               "3: Wrapper validator and support");
    }

    // Case 4: Wrapper alpha = 0.25, switches to rebuild.
    {
        Graph graph;
        if (!graph.load("data/test/fig1.txt")) return 1;

        DDFLPD_WL wl(1, 4, false);
        if (wl.initialize(graph) != Status::Ok) return 1;

        ReplayCounts c = Replay::wrapperBatch(
            graph, makeBatch(0, 1, 5), wl
        );

        checkCounts(c, 1, 1, 7, 0, 24, 33, true,
                    "4: Wrapper alpha 0.25 counts");

        expect(validWrapper(graph, wl),
               "4: Wrapper fallback validator and support");
    }

    // Case 5: Wrapper alpha = infinity, never switches.
    {
        Graph graph;
        if (!graph.load("data/test/fig1.txt")) return 1;

        DDFLPD_WL wl(0, 1, true);
        if (wl.initialize(graph) != Status::Ok) return 1;

        ReplayCounts c = Replay::wrapperBatch(
            graph, makeBatch(0, 1, 5), wl
        );

        checkCounts(c, 1, 1, 17, 0, 0, 19, false,
                    "5: Wrapper infinity counts");

        expect(validWrapper(graph, wl),
               "5: Wrapper infinity validator and support");
    }

    // Case 6: Two batches on the same baseline object.
    {
        Graph graph;
        if (!graph.load("data/test/fig1.txt")) return 1;

        DDFLPD core;
        if (core.initialize(graph) != Status::Ok) return 1;

        // First update: S-A from 1 to 5.
        ReplayCounts first = Replay::baselineBatch(
            graph, makeBatch(0, 1, 5), core
        );

        checkCounts(first, 1, 1, 17, 0, 0, 19, false,
                    "6: First batch counts");

        expect(validBaseline(graph, core),
               "6: First batch validator and support");

        // B-C is now a tree edge.
        // Second update: B-C from 4 to 8.
        ReplayCounts second = Replay::baselineBatch(
            graph, makeBatch(3, 4, 8), core
        );

        checkCounts(second, 1, 1, 8, 0, 0, 10, false,
                    "6: Second batch counts");

        expect(validBaseline(graph, core),
               "6: Second batch validator and support");
    }

    // Case 7: B-C is initially a non-tree edge.
    {
        Graph graph;
        if (!graph.load("data/test/fig1.txt")) return 1;

        DDFLPD core;
        if (core.initialize(graph) != Status::Ok) return 1;

        ReplayCounts c = Replay::baselineBatch(
            graph, makeBatch(3, 4, 8), core
        );

        checkCounts(c, 1, 0, 0, 0, 0, 1, false,
                    "7: Non-tree edge counts");

        expect(validBaseline(graph, core),
               "7: Non-tree edge validator and support");
    }

    // Show the final test result.
    if (failures != 0) {
        std::cerr << failures << " Replay checks failed.\n";
        return 1;
    }

    std::cout << "All 7 Replay cases passed.\n";
    return 0;
}

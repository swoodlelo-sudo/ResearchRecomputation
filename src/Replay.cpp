
#include "Replay.h"
#include "Dijkstra.h"

#include <limits>

// Add a count without overflowing.
bool Replay::addChecked(unsigned long long& sum,
                        unsigned long long x) {

    if (x > std::numeric_limits<unsigned long long>::max() - sum) {
        return false;
    }

    sum += x;
    return true;
}


// Count updated edges that are in the current tree.
unsigned long long Replay::countTreeEdges(
    const Graph& graph,
    const Batch& batch,
    const std::vector<int>& parent) {

    unsigned long long count = 0;

    for (const auto& update : batch.updates) {
        const auto& edge = graph.getEdge(update.edgeId);

        int u = edge.u;
        int v = edge.v;

        if (parent[v] == u || parent[u] == v) {
            ++count;
        }
    }

    return count;
}


// Method 1: Full Dijkstra.
ReplayCounts Replay::dijkstraBatch(
    Graph& graph,
    const Batch& batch,
    MinHeap& Q,
    std::vector<long long>& dist,
    std::vector<int>& parent) {

    ReplayCounts counts;

    // Apply all weight increases.
    for (const auto& update : batch.updates) {
        graph.setWeight(update.edgeId, update.newWeight);
    }

    // Run Dijkstra with counting enabled.
    unsigned long long W = 0;

    Status result = Dijkstra::run(
        graph, Q, dist, parent, W, true
    );

    if (result != Status::Ok) {
        counts.status = Status::Overflow;
        return counts;
    }

    counts.dijkstra = W;
    counts.total = W;

    return counts;
}


// Method 2: Baseline DDFLP-D.
ReplayCounts Replay::baselineBatch(
    Graph& graph,
    const Batch& batch,
    DDFLPD& core) {

    ReplayCounts counts;

    // One stored-edge update per changed edge.
    counts.apply = batch.updates.size();

    // Count tree edges BEFORE repair changes the parents.
    counts.queueSetup = countTreeEdges(
        graph, batch, core.parents()
    );

    // Run repair with counting, but no work limit.
    RepairResult result = core.repair(
        graph,
        batch,
        true,
        std::numeric_limits<unsigned long long>::max()
    );

    // No cleanup needed run_Replay stops this sequence
    if (result != RepairResult::Finished) {
        counts.status = Status::Overflow;
        return counts;
    }

    counts.repair = core.lastWR();

    // W_total = apply + queueSetup + repair.
    if (!addChecked(counts.total, counts.apply) ||
        !addChecked(counts.total, counts.queueSetup) ||
        !addChecked(counts.total, counts.repair)) {

        counts.status = Status::Overflow;
        return counts;
    }

    return counts;
}


// Method 3: DDFLP-D with work limit.
ReplayCounts Replay::wrapperBatch(
    Graph& graph,
    const Batch& batch,
    DDFLPD_WL& wl) {

    ReplayCounts counts;

    // Count stored-edge updates.
    counts.apply = batch.updates.size();

    // Use the current parents before updating.
    counts.queueSetup = countTreeEdges(
        graph, batch, wl.repairCore().parents()
    );

    // Run the wrapper with its chosen alpha.
    auto outcome = wl.update(graph, batch);

    if (outcome.status != Status::Ok) {
        counts.status = Status::Overflow;
        return counts;
    }

    counts.repair = outcome.wR;
    counts.restart = outcome.wRestart;
    counts.switched = outcome.switched;

    // W_total includes the repair and any fallback.
    if (!addChecked(counts.total, counts.apply) ||
        !addChecked(counts.total, counts.queueSetup) ||
        !addChecked(counts.total, counts.repair) ||
        !addChecked(counts.total, counts.restart)) {

        counts.status = Status::Overflow;
        return counts;
    }

    return counts;
}

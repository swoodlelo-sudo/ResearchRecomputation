
#ifndef REPLAY_H
#define REPLAY_H

/*
    Step 13: W_total Replay

    Reference: phe 6 corrected, Table 3, pp. 4-5.

    W_total counts all operations for a batch:
    apply + queueSetup + repair + dijkstra + restart.

    This is a separate, untimed replay.
    It does not change the timed code.
*/

#include "Graph.h"
#include "MinHeap.h"
#include "BatchGen.h"
#include "DDFLPD.h"
#include "DDFLPD_WL.h"
#include "Status.h"

#include <vector>

// Counts for one batch.
struct ReplayCounts {
    Status status = Status::Ok;

    unsigned long long apply = 0;
    unsigned long long queueSetup = 0;
    unsigned long long repair = 0;
    unsigned long long dijkstra = 0;
    unsigned long long restart = 0;
    unsigned long long total = 0;

    bool switched = false;
};

class Replay {
public:

    // Count updated tree edges.
    [[nodiscard]] static unsigned long long countTreeEdges(
        const Graph& graph,
        const Batch& batch,
        const std::vector<int>& parent
    );

    // Full Dijkstra count.
    [[nodiscard]] static ReplayCounts dijkstraBatch(
        Graph& graph,
        const Batch& batch,
        MinHeap& Q,
        std::vector<long long>& dist,
        std::vector<int>& parent
    );

    // Baseline repair count.
    [[nodiscard]] static ReplayCounts baselineBatch(
        Graph& graph,
        const Batch& batch,
        DDFLPD& core
    );

    // Wrapper repair and fallback count.
    [[nodiscard]] static ReplayCounts wrapperBatch(
        Graph& graph,
        const Batch& batch,
        DDFLPD_WL& wl
    );

private:

    // Prevent overflow when adding counts.
    [[nodiscard]] static bool addChecked(
        unsigned long long& sum,
        unsigned long long x
    );
};

#endif

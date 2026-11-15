#ifndef DDFLPD_H
#define DDFLPD_H

#include "BatchGen.h"
#include "Graph.h"
#include "MinHeap.h"
#include "Status.h"

#include <cstddef>
#include <vector>

enum class RepairResult { Finished, LimitExceeded, Overflow };

// Each edge is stored at one endpoint, chosen by how many edges it owns so
// far. Each owner's queue is a min-heap of edge IDs, keyed by the live weight.
// These queues are support data; they do not add to the repair work count.
class EdgeStore {
public:
    void clear(int n, int m);

    // Called once per edge, in edge-ID order, before add(). Ties use smaller ID.
    int chooseOwner(int u, int v);

    void add(int e, int owner, const Graph& graph);

    // Call after increasing the edge's weight in the graph.
    void increaseKey(int e, const Graph& graph);

    // Untimed: check owners, counts, positions, uniqueness, and heap order.
    bool check(const Graph& graph) const;

    int ownerOf(int e) const;

private:
    std::vector<int> owner_;
    std::vector<int> ownedCount_;
    std::vector<std::vector<int>> heapOf_;
    std::vector<int> heapPos_;

    bool less(int a, int b, const Graph& graph) const;
    void swapAt(int vertex, int a, int b);
    void siftUp(int vertex, int position, const Graph& graph);
    void siftDown(int vertex, int position, const Graph& graph);
};

// DDFLP-D without deletion. One object keeps one method's state across batches.
// Use a connected, simple graph with positive weights and a fixed source.
// The same repair is used with counting off, or with counting and a work limit.
class DDFLPD {
public:
    // Algorithm 0: build the initial tree and support, storing W_D and W_B.
    Status initialize(const Graph& graph);

    // Algorithm 5: rebuild child lists and stored-edge queues. W_B = 2m.
    Status buildSupport(const Graph& graph, unsigned long long& work);

    // The graph must still match this object's tree and support. Batch edge
    // IDs must be distinct, oldWeight must match, and newWeight must increase.
    // Phase 1 applies the whole batch before any counted repair operation.
    // A stopped repair is incomplete: call abandonRepair before rebuilding.
    RepairResult repair(Graph& graph, const Batch& batch, bool counting,
                        unsigned long long limit);

    // Clear Q and reset only vertices colored in this batch. Keep the metrics.
    void abandonRepair();

    // Call after abandonRepair. Reuse the empty Q; return W_restart in work.
    // getWD/getWB keep the initial calibration; work gives this rebuild's cost.
    Status rebuild(const Graph& graph, unsigned long long& work);

    // Untimed: check the child lists, white colors, empty Q and stored edges.
    bool checkSupport(const Graph& graph) const;

    const std::vector<long long>& distances() const;
    const std::vector<int>& parents() const;
    unsigned long long getWD() const;
    unsigned long long getWB() const;
    unsigned long long lastWR() const;
    std::size_t lastAffected() const;
    int lastRed() const;
    unsigned long long lastDegreeSum(const Graph& graph) const;
    int edgeOwner(int edgeId) const;

private:
    std::vector<long long> distance_;
    std::vector<int> parent_;
    MinHeap Q_;

    std::vector<char> color_;  // 0 white, 1 pink, 2 red
    std::vector<int> colored_; // Kept until next batch for |A| and degree sum.

    std::vector<int> firstChild_;
    std::vector<int> nextSibling_;
    std::vector<int> prevSibling_;
    std::vector<int> listOwner_;

    EdgeStore store_;

    bool counting_ = false;
    unsigned long long limit_ = 0;
    unsigned long long wR_ = 0;
    unsigned long long wD_ = 0;
    unsigned long long wB_ = 0;
    RepairResult stopReason_ = RepairResult::Finished;
    int redCount_ = 0;

    void unlink(int v);
    void link(int v, int p);
    void moveTo(int v, int p);

    // Called after a counted operation. Equal to limit keeps repairing.
    bool charge();

    // Helpers return false only to stop repair; checkPink writes its answer
    // separately so "not pink" is different from "stop now".
    bool checkPink(const Graph& graph, int z, bool& pink);
    bool checkBestNonRed(const Graph& graph, int z);
    bool processRed(const Graph& graph, int z);
};

#endif
#ifndef DDFLPD_WL_VALIDATOR_H
#define DDFLPD_WL_VALIDATOR_H

#include "Graph.h"
#include "MinHeap.h"

#include <vector>

// VALIDATOR
//
// Checks a method's output after a batch. This always runs OUTSIDE the timer.
//
// The reference answer comes from a fresh Dijkstra::run with counting off.
// It uses its own distance and parent vectors and a separate reference heap,
// so it does not change the method's output or its queue.
// This is a separate run of the same Dijkstra implementation.
//
// What is checked:
//     1. exact distances against a fresh run on the same updated graph
//     2. the source has no parent
//     3. every other reached vertex has a valid parent that is a real neighbor
//        with a tight edge: d(v) = d(p(v)) + w(p(v), v)
//     4. every unreached vertex has no parent
//
// Parents do NOT have to match the reference. When two shortest paths tie,
// different valid trees are allowed. With positive weights, tight parent
// edges lead to smaller distances, so following parents cannot form a cycle.
class Validator {
public:
    // Set up referenceQ for the graph's vertex count before calling.
    // It must be empty and separate from the method's Q.
    // The graph must satisfy Dijkstra's positive-weight and distance limits.
    // Returns true on success. Otherwise prints the first problem and
    // returns false. Both output vectors must have one entry per vertex.
    [[nodiscard]] static bool validateOutput(
        const Graph& graph,
        const std::vector<long long>& distance,
        const std::vector<int>& parent,
        MinHeap& referenceQ);
};

#endif
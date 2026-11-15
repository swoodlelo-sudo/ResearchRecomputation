#ifndef DDFLPD_WL_BATCHGEN_H
#define DDFLPD_WL_BATCHGEN_H

#include "Graph.h"
#include "Status.h"

#include <array>
#include <string>
#include <vector>

inline constexpr int BATCH_COUNT = 30;
inline constexpr int BATCHES_PER_CODE = 10;

// One edge update. Keep both weights so the same batch can be replayed.
struct BatchUpdate{
    int edgeId;
    long long oldWeight;
    long long newWeight;
};

// One batch in the sequence. IDs go from 0 to 29.
// Codes 1, 10, and 100 mean 0.1%, 1%, and 10% of the graph's edges.
struct Batch{
    int batchId;
    int code;
    std::vector<BatchUpdate> updates;
};

// The full sequence, including the seed and the shuffled order of the codes.
struct BatchSequence{
    unsigned long long seed = 0;
    int edgeCount = 0;
    std::array<int, BATCH_COUNT> codes{};
    std::vector<Batch> batches;
};

// Reads a saved batch file back into sequence and checks it.
//
// Structure checks:
//     the header has m > 0 and exactly 30 batches
//     the 30 stored codes are only 1, 10, or 100, with 10 of each
//     batch IDs go 0 to 29 in order
//     each batch's code matches the stored code order
//     each batch's k matches k = max(1, floor(p * m + 0.5))
//     edge IDs are in range and strictly increasing within a batch,
//     which means sorted and distinct
//
// Weight checks:
//     every weight is positive and newWeight is twice oldWeight
//     when an edge appears again, its oldWeight must equal the newWeight
//     from its previous appearance in the sequence
//
// This creates the batches that each method will receive in the experiments.
// It works on a local copy of the weights, so the original graph is unchanged.
// Later batches use the weights left by earlier batches in the sequence.
class BatchGenerator{
public:
    // The graph must have at least one edge and positive weights.
    // Uses one mt19937_64 generator, seeded once for the whole sequence.
    // Makes 30 batches, with 10 of each code, and sorted distinct edge IDs
    // within each batch. The same edge can appear again in a later batch.
    // Returns Overflow if a chosen edge's weight cannot be doubled safely.
    // On Overflow, sequence is incomplete and must not be used.
    [[nodiscard]] static Status generate(
        const Graph &graph,
        unsigned long long seed,
        BatchSequence &sequence);

    // File format:
    //     seed m 30
    //     <30 codes>
    //     batchId code k
    //     edgeId oldWeight newWeight    (k lines per batch)
    // The destination folder must already exist.
    // Returns false if the file cannot be opened or written successfully.
    [[nodiscard]] static bool save(
        const BatchSequence &sequence,
        const std::string &fileName);

    // Weights must be positive, with newWeight equal to twice oldWeight.
    // When an edge appears again, its oldWeight must equal the newWeight
    // recorded for its previous appearance in the sequence.
    // The first appearance cannot be checked against the graph here.
    // The driver checks oldWeight against the graph when applying each update.
    // Returns false for a read or validation failure. Only use sequence
    // when this returns true.
    [[nodiscard]] static bool load(
        const std::string &fileName,
        BatchSequence &sequence);
};

#endif
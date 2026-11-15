// Test the batch generator before using its files in the experiments.

#include "BatchGen.h"

#include <cstddef>
#include <iostream>
#include <vector>

// Compare every stored field, including the order of batches and updates.
static bool sameSequence(const BatchSequence& a, const BatchSequence& b) {
    if (a.seed != b.seed || a.edgeCount != b.edgeCount ||
        a.codes != b.codes || a.batches.size() != b.batches.size()) {
        return false;
    }

    for (std::size_t i = 0; i < a.batches.size(); i++) {
        const Batch& first = a.batches[i];
        const Batch& second = b.batches[i];

        if (first.batchId != second.batchId || first.code != second.code ||
            first.updates.size() != second.updates.size()) {
            return false;
        }

        for (std::size_t j = 0; j < first.updates.size(); j++) {
            const BatchUpdate& x = first.updates[j];
            const BatchUpdate& y = second.updates[j];

            if (x.edgeId != y.edgeId || x.oldWeight != y.oldWeight ||
                x.newWeight != y.newWeight) {
                return false;
            }
        }
    }

    return true;
}

static bool report(const int number, const char* description, const bool passed) {
    std::cout << (passed ? "PASS " : "FAIL ") << number << ": "
              << description << "\n";
    return passed;
}

int main() {
    // This path graph is only for testing: 1001 vertices and 1000 edges.
    Graph graph(1001, 0);
    for (int i = 0; i < 1000; i++) {
        graph.addEdge(i, i + 1, 1);
    }

    const unsigned long long seed = 17400;
    BatchSequence sequence;
    const Status status = BatchGenerator::generate(graph, seed, sequence);

    // 1. Check the status, batch counts, code counts, and expected sizes.
    bool shapeCorrect = status == Status::Ok && sequence.seed == seed &&
                        sequence.edgeCount == 1000 &&
                        sequence.batches.size() == BATCH_COUNT;
    int count1 = 0;
    int count10 = 0;
    int count100 = 0;

    if (shapeCorrect) {
        for (int b = 0; b < BATCH_COUNT; b++) {
            const Batch& batch = sequence.batches[b];
            int expectedSize = 0;

            if (batch.code == 1) {
                count1++;
                expectedSize = 1;
            } else if (batch.code == 10) {
                count10++;
                expectedSize = 10;
            } else if (batch.code == 100) {
                count100++;
                expectedSize = 100;
            } else {
                shapeCorrect = false;
            }

            if (batch.batchId != b || batch.code != sequence.codes[b] ||
                batch.updates.size() != static_cast<std::size_t>(expectedSize)) {
                shapeCorrect = false;
            }
        }
    }

    shapeCorrect = shapeCorrect && count1 == BATCHES_PER_CODE &&
                   count10 == BATCHES_PER_CODE && count100 == BATCHES_PER_CODE;

    if (!report(1, "Ok, 30 batches, 10 of each code, sizes 1/10/100",
                shapeCorrect)) {
        return 1;
    }

    // 2. Each batch must have valid, sorted, distinct edge IDs.
    bool idsCorrect = true;
    for (const Batch& batch : sequence.batches) {
        int previous = -1;

        for (const BatchUpdate& update : batch.updates) {
            if (update.edgeId < 0 || update.edgeId >= 1000 ||
                update.edgeId <= previous) {
                idsCorrect = false;
            }
            previous = update.edgeId;
        }
    }

    if (!report(2, "edge IDs are in range, sorted, and distinct", idsCorrect)) {
        return 1;
    }

    // 3. Replay the updates from the original weights of 1.
    std::vector<long long> replay(1000, 1);
    bool chainCorrect = true;

    for (const Batch& batch : sequence.batches) {
        for (const BatchUpdate& update : batch.updates) {
            if (update.oldWeight <= 0 ||
                update.oldWeight > Graph::INF_DISTANCE / 2 ||
                update.oldWeight != replay[update.edgeId] ||
                update.newWeight != 2 * update.oldWeight) {
                chainCorrect = false;
                break;
            }
            replay[update.edgeId] = update.newWeight;
        }

        if (!chainCorrect) {
            break;
        }
    }

    if (!report(3, "old/new weights chain correctly across batches",
                chainCorrect)) {
        return 1;
    }

    // 4. Repeat with the same seed and compare every stored field.
    BatchSequence repeated;
    const Status repeatedStatus =
        BatchGenerator::generate(graph, seed, repeated);

    if (!report(4, "same seed gives an identical sequence",
                repeatedStatus == Status::Ok &&
                sameSequence(sequence, repeated))) {
        return 1;
    }

    // 5. Generation must not change the original graph.
    bool graphUnchanged = true;
    for (int edgeId = 0; edgeId < graph.getEdgeCount(); edgeId++) {
        if (graph.getWeight(edgeId) != 1) {
            graphUnchanged = false;
        }
    }

    if (!report(5, "the graph's weights are unchanged", graphUnchanged)) {
        return 1;
    }

    // 6. The data/batches folder must exist before running this test.
    const char* fileName = "data/batches/test_seed_17400.txt";
    BatchSequence loaded;

    const bool roundTrip = BatchGenerator::save(sequence, fileName) &&
                           BatchGenerator::load(fileName, loaded) &&
                           sameSequence(sequence, loaded);

    if (!report(6, "save/load gives an identical sequence", roundTrip)) {
        return 1;
    }

    // 7. One edge guarantees that the oversized weight is selected.
    Graph largeWeight(2, 0);
    const long long tooLarge = Graph::INF_DISTANCE / 2 + 1;
    largeWeight.addEdge(0, 1, tooLarge);

    BatchSequence overflowSequence;
    const Status overflowStatus =
        BatchGenerator::generate(largeWeight, seed, overflowSequence);

    if (!report(7, "doubling an oversized weight returns Overflow",
                overflowStatus == Status::Overflow &&
                largeWeight.getWeight(0) == tooLarge)) {
        return 1;
    }

    return 0;
}
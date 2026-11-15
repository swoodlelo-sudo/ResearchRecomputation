#include "BatchGen.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <fstream>
#include <numeric>
#include <random>

// Both generate and load use the same rounding rule.
// static keeps this helper private to this file.
static int batchSize(const int m, const int code) {
    const double proportion = code / 1000.0;
    return std::max(1, static_cast<int>(std::floor(proportion * m + 0.5)));
}

Status BatchGenerator::generate(const Graph& graph,
                                const unsigned long long seed,
                                BatchSequence& sequence) {
    const int m = graph.getEdgeCount();
    assert(m > 0);

    // Start fresh so this does not append to an older sequence.
    sequence.seed = seed;
    sequence.edgeCount = m;
    sequence.batches.clear();
    sequence.batches.reserve(BATCH_COUNT);

    // Seed once, then use this same generator for every shuffle.
    std::mt19937_64 rng(seed);

    for (int i = 0; i < BATCHES_PER_CODE; i++) {
        sequence.codes[i] = 1;
        sequence.codes[i + BATCHES_PER_CODE] = 10;
        sequence.codes[i + 2 * BATCHES_PER_CODE] = 100;
    }
    std::shuffle(sequence.codes.begin(), sequence.codes.end(), rng);

    // Work on a copy. The original graph's weights stay unchanged.
    std::vector<long long> weights(m);
    for (int edgeId = 0; edgeId < m; edgeId++) {
        weights[edgeId] = graph.getWeight(edgeId);
    }

    // Allocate the list once, but reset its order before each shuffle.
    std::vector<int> ids(m);

    for (int b = 0; b < BATCH_COUNT; b++) {
        const int code = sequence.codes[b];
        const int k = batchSize(m, code);

        std::iota(ids.begin(), ids.end(), 0);
        std::shuffle(ids.begin(), ids.end(), rng);
        std::sort(ids.begin(), ids.begin() + k);

        Batch batch;
        batch.batchId = b;
        batch.code = code;
        batch.updates.reserve(k);

        for (int i = 0; i < k; i++) {
            const int edgeId = ids[i];
            const long long oldWeight = weights[edgeId];

            // Check before multiplying, so the doubling itself cannot overflow.
            if (oldWeight > Graph::INF_DISTANCE / 2) {
                return Status::Overflow;
            }

            weights[edgeId] = 2 * oldWeight;
            batch.updates.push_back(
                BatchUpdate{edgeId, oldWeight, weights[edgeId]});
        }

        sequence.batches.push_back(batch);
    }

    return Status::Ok;
}

bool BatchGenerator::save(const BatchSequence& sequence,
                          const std::string& fileName) {
    std::ofstream out(fileName);
    if (!out) {
        return false;
    }

    out << sequence.seed << " " << sequence.edgeCount << " "
        << BATCH_COUNT << "\n";

    for (int i = 0; i < BATCH_COUNT; i++) {
        if (i > 0) {
            out << " ";
        }
        out << sequence.codes[i];
    }
    out << "\n";

    for (const Batch& batch : sequence.batches) {
        out << batch.batchId << " " << batch.code << " "
            << batch.updates.size() << "\n";

        for (const BatchUpdate& update : batch.updates) {
            out << update.edgeId << " " << update.oldWeight << " "
                << update.newWeight << "\n";
        }
    }

    out.close();
    return !out.fail();
}

bool BatchGenerator::load(const std::string& fileName,
                          BatchSequence& sequence) {
    std::ifstream in(fileName);
    if (!in) {
        return false;
    }

    // Remove any older sequence before filling this one.
    sequence = BatchSequence{};

    int batchCount = 0;
    if (!(in >> sequence.seed >> sequence.edgeCount >> batchCount)) {
        return false;
    }
    if (sequence.edgeCount <= 0 || batchCount != BATCH_COUNT) {
        return false;
    }

    const int m = sequence.edgeCount;
    int count1 = 0;
    int count10 = 0;
    int count100 = 0;

    for (int& code : sequence.codes) {
        if (!(in >> code)) {
            return false;
        }

        if (code == 1) {
            count1++;
        } else if (code == 10) {
            count10++;
        } else if (code == 100) {
            count100++;
        } else {
            return false;
        }
    }

    if (count1 != BATCHES_PER_CODE || count10 != BATCHES_PER_CODE ||
        count100 != BATCHES_PER_CODE) {
        return false;
    }

    // Keep this across all batches. Zero means the edge has not been seen.
    std::vector<long long> lastWeight(m, 0);
    sequence.batches.reserve(BATCH_COUNT);

    for (int b = 0; b < BATCH_COUNT; b++) {
        Batch batch;
        int k = 0;
        if (!(in >> batch.batchId >> batch.code >> k)) {
            return false;
        }
        if (batch.batchId != b || batch.code != sequence.codes[b] ||
            k != batchSize(m, batch.code)) {
            return false;
        }

        batch.updates.reserve(k);
        int previousEdgeId = -1;

        for (int i = 0; i < k; i++) {
            BatchUpdate update;
            if (!(in >> update.edgeId >> update.oldWeight >> update.newWeight)) {
                return false;
            }

            // Strictly increasing IDs means sorted and distinct.
            if (update.edgeId < 0 || update.edgeId >= m ||
                update.edgeId <= previousEdgeId) {
                return false;
            }

            if (update.oldWeight <= 0 ||
                update.oldWeight > Graph::INF_DISTANCE / 2) {
                return false;
            }
            if (update.newWeight != 2 * update.oldWeight) {
                return false;
            }

            // A repeated edge must continue from its previous new weight.
            if (lastWeight[update.edgeId] != 0 &&
                update.oldWeight != lastWeight[update.edgeId]) {
                return false;
            }

            lastWeight[update.edgeId] = update.newWeight;
            previousEdgeId = update.edgeId;
            batch.updates.push_back(update);
        }

        sequence.batches.push_back(batch);
    }

    return true;
}
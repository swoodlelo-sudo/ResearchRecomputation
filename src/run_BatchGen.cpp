
#include "BatchGen.h"
#include "Graph.h"

#include <filesystem>
#include <iostream>
#include <string>

int main() {
    const int sizes[] = {10000, 30000, 90000, 270000, 810000};
    const unsigned long long seeds[] = {17400, 17401, 17402, 17403};

    // Create the output folder.
    std::filesystem::create_directories("data/batches");

    for (int N : sizes) {
        Graph graph;
        const std::string graphFile =
            "data/graphs/NW_" + std::to_string(N) + ".txt";

        if (!graph.load(graphFile)) {
            std::cerr << "Could not load " << graphFile << '\n';
            return 1;
        }

        for (unsigned long long seed : seeds) {
            BatchSequence sequence;
            Status result = BatchGenerator::generate(graph, seed, sequence);

            if (result != Status::Ok) {
                std::cerr << "Generation failed: N=" << N
                          << " seed=" << seed << '\n';
                return 1;
            }

            const std::string batchFile =
                "data/batches/NW_" + std::to_string(N) +
                "_seed" + std::to_string(seed) + ".txt";

            if (!BatchGenerator::save(sequence, batchFile)) {
                std::cerr << "Could not save " << batchFile << '\n';
                return 1;
            }

            // Read back the saved file.
            BatchSequence loaded;
            if (!BatchGenerator::load(batchFile, loaded)) {
                std::cerr << "Could not reload " << batchFile << '\n';
                return 1;
            }

            if (loaded.seed != seed ||
                loaded.edgeCount != graph.getEdgeCount() ||
                loaded.batches.size() != 30) {
                std::cerr << "Batch file mismatch: " << batchFile << '\n';
                return 1;
            }

            unsigned long long totalUpdates = 0;
            for (const Batch& batch : loaded.batches) {
                totalUpdates += batch.updates.size();
            }

            std::cout << "N=" << N
                      << " seed=" << seed
                      << " totalUpdates=" << totalUpdates << '\n';
        }
    }

    return 0;
}

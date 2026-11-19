
#include "Replay.h"
#include "Graph.h"
#include "BatchGen.h"
#include "Dijkstra.h"
#include "DDFLPD.h"
#include "DDFLPD_WL.h"
#include "Validator.h"
#include "MinHeap.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using ULL = unsigned long long;

// Keep this table the same as Driver.cpp
enum class MethodType {
    Dijkstra,
    Baseline,
    Wrapper
};

struct Configuration {
    const char* name;
    MethodType type;
    int num;
    int den;
    bool infinite;
    const char* alpha;
};

const Configuration configs[] = {
    // Dijkstra and Baseline use legal placeholder values
    {"Dijkstra", MethodType::Dijkstra, 1, 1, false, "NA"},
    {"Baseline", MethodType::Baseline, 1, 1, false, "NA"},
    {"WL_1", MethodType::Wrapper, 1, 1, false, "1"},
    {"WL_075", MethodType::Wrapper, 3, 4, false, "0.75"},
    {"WL_05", MethodType::Wrapper, 1, 2, false, "0.5"},
    {"WL_025", MethodType::Wrapper, 1, 4, false, "0.25"},
    {"WL_inf", MethodType::Wrapper, 0, 1, true, "inf"}
};

const int graphSizes[] = {
    10000, 30000, 90000, 270000, 810000
};

constexpr int batchCount = 30;
constexpr int configCount = 7;


// Check the graph size
bool validSize(int N) {
    for (int size : graphSizes) {
        if (N == size) {
            return true;
        }
    }

    return false;
}


// Get the batch percentage
const char* batchPercent(int code) {
    if (code == 1) return "0.1";
    if (code == 10) return "1";
    if (code == 100) return "10";

    return "NA";
}


// Check the old weights before updating
bool checkOldWeights(const Graph& graph, const Batch& batch) {
    for (const auto& update : batch.updates) {
        if (graph.getWeight(update.edgeId) != update.oldWeight) {
            return false;
        }
    }

    return true;
}


// Information for one configuration
struct RowInfo {
    std::string label;
    std::string graphFile;
    int seed;
    const Configuration* config;
    std::string commit;
};


// Write the CSV header
void writeHeader(std::ofstream& out) {
    out << "label,graph_file,seed,config,alpha,"
        << "batch_id,batch_pct,batch_edges,"
        << "W_apply,W_queue_setup,W_repair,W_dijkstra,"
        << "W_restart,W_total,switched,outcome,commit\n";
}


// Write one batch result
void writeRow(std::ofstream& out,
              const RowInfo& info,
              const Batch& batch,
              int batchId,
              const ReplayCounts* counts,
              const std::string& outcome) {

    out << info.label << ','
        << info.graphFile << ','
        << info.seed << ','
        << info.config->name << ','
        << info.config->alpha << ','
        << batchId << ','
        << batchPercent(batch.code) << ','
        << batch.updates.size() << ',';

    if (counts != nullptr) {
        out << counts->apply << ','
            << counts->queueSetup << ','
            << counts->repair << ','
            << counts->dijkstra << ','
            << counts->restart << ','
            << counts->total << ','
            << (counts->switched ? "yes" : "no");
    } else {
        // No counts if the batch did not finish
        out << "NA,NA,NA,NA,NA,NA,NA";
    }

    out << ',' << outcome << ','
        << info.commit << '\n';

    // Save the row
    out.flush();
}


// Mark the remaining batches as NOT_RUN
void writeNotRun(std::ofstream& out,
                 const RowInfo& info,
                 const BatchSequence& sequence,
                 int firstBatch) {

    for (int b = firstBatch; b < batchCount; ++b) {
        writeRow(out, info, sequence.batches[b],
                 b, nullptr, "NOT_RUN");
    }
}


int main(int argc, char* argv[]) {

    // Check the arguments
    if (argc != 4) {
        std::cerr
            << "Usage: ./build/run_Replay.exe "
            << "<N> <pilot|final> <commit>\n";
        return 1;
    }

    int N = 0;

    try {
        std::string argument = argv[1];
        std::size_t used = 0;

        N = std::stoi(argument, &used);

        if (used != argument.size()) {
            throw std::invalid_argument("Invalid N");
        }

    } catch (const std::exception&) {
        std::cerr << "Invalid graph size.\n";
        return 1;
    }

    std::string mode = argv[2];
    std::string commit = argv[3];

    if (!validSize(N) ||
        (mode != "pilot" && mode != "final") ||
        commit.size() != 7) {

        std::cerr
            << "Usage: ./build/run_Replay.exe "
            << "<N> <pilot|final> <commit>\n";
        return 1;
    }

    // Choose the seeds
    std::vector<int> seeds;

    if (mode == "pilot") {
        seeds = {17400};
    } else {
        seeds = {17401, 17402, 17403};
    }

    std::string label = (mode == "pilot") ? "PILOT" : "FINAL";

    const std::string stem = "NE_" + std::to_string(N);
    const std::string graphFile = stem + ".txt";

    // Load the original graph
    Graph original;

    if (!original.load("data/graphs/" + graphFile)) {
        std::cerr << "Could not load " << graphFile << '\n';
        return 1;
    }

    if (original.getVertexCount() != N) {
        std::cerr << "Graph size mismatch.\n";
        return 1;
    }

    // Load the batch files
    std::vector<BatchSequence> sequences;

    for (int seed : seeds) {
        BatchSequence sequence;

        const std::string batchFile =
            "data/batches/" + stem +
            "_seed" + std::to_string(seed) + ".txt";

        if (!BatchGenerator::load(batchFile, sequence)) {
            std::cerr << "Could not load " << batchFile << '\n';
            return 1;
        }

        // Check that the batches match the graph
        if (sequence.seed != static_cast<ULL>(seed) ||
            sequence.edgeCount != original.getEdgeCount() ||
            sequence.batches.size() != batchCount) {

            std::cerr << "Batch file mismatch: "
                      << batchFile << '\n';
            return 1;
        }

        sequences.push_back(std::move(sequence));
    }

    // Create the results folder
    std::filesystem::create_directories("Results");

    const std::string outputFile =
        "Results/wtotal_" + stem + "_" + mode + ".csv";

    std::ofstream csv(outputFile);

    if (!csv) {
        std::cerr << "Could not create " << outputFile << '\n';
        return 1;
    }

    writeHeader(csv);
    csv.flush();

    // Save the compiler and run information
    const std::string infoFile =
        "Results/runinfo_replay_" + stem + "_" + mode + ".txt";

    std::ofstream runInfo(infoFile);

    if (!runInfo) {
        std::cerr << "Could not create " << infoFile << '\n';
        return 1;
    }

    runInfo << "Compiler: " << __VERSION__ << '\n';
    runInfo << "Flags: -std=c++17 -O2 -Wall -Wextra\n";
    runInfo << "Commit: " << commit << '\n';
    runInfo << "N: " << N << '\n';
    runInfo << "Mode: " << mode << '\n';
    runInfo << "Graph: " << graphFile << '\n';

    runInfo.close();

    if (runInfo.fail()) {
        std::cerr << "Could not finish " << infoFile << '\n';
        return 1;
    }

    bool allPassed = true;

    // Run each seed
    for (std::size_t j = 0; j < seeds.size(); ++j) {

        int seed = seeds[j];
        const BatchSequence& sequence = sequences[j];

        // Store baseline counts for the infinity check
        std::vector<ULL> baselineTotals(batchCount, 0);
        std::vector<bool> baselineValid(batchCount, false);

        // Run all seven configurations
        for (int k = 0; k < configCount; ++k) {

            const Configuration& config = configs[k];

            RowInfo info{
                label, graphFile, seed, &config, commit
            };

            // Start each method with a fresh graph
            Graph work = original;

            DDFLPD baseline;
            DDFLPD_WL wl(config.num, config.den, config.infinite);

            MinHeap Q;
            MinHeap refQ;

            std::vector<long long> dist;
            std::vector<int> parent;

            ULL W = 0;
            Status initStatus = Status::Ok;

            // Initialize the selected method
            if (config.type == MethodType::Dijkstra) {
                Q.setup(work.getVertexCount());

                initStatus = Dijkstra::run(
                    work, Q, dist, parent, W, false
                );

            } else if (config.type == MethodType::Baseline) {
                initStatus = baseline.initialize(work);

            } else {
                initStatus = wl.initialize(work);
            }

            // Set up the validator queue
            refQ.setup(work.getVertexCount());

            if (initStatus != Status::Ok) {

                // Record the failure on batch 0
                writeRow(csv, info, sequence.batches[0],
                         0, nullptr, "OVERFLOW");

                writeNotRun(csv, info, sequence, 1);

                std::cout << label
                          << " seed=" << seed
                          << " config=" << config.name
                          << " outcome=OVERFLOW\n";

                allPassed = false;
                continue;
            }

            std::string finalOutcome = "PASS";

            // Process all 30 batches
            for (int b = 0; b < batchCount; ++b) {

                const Batch& batch = sequence.batches[b];

                // Check the weights before the update
                if (!checkOldWeights(work, batch)) {
                    std::cerr << "Old weight mismatch: N=" << N
                              << " seed=" << seed
                              << " config=" << config.name
                              << " batch=" << b << '\n';
                    return 1;
                }

                ReplayCounts counts;

                // Run the selected replay
                if (config.type == MethodType::Dijkstra) {

                    counts = Replay::dijkstraBatch(
                        work, batch, Q, dist, parent
                    );

                } else if (config.type == MethodType::Baseline) {

                    counts = Replay::baselineBatch(
                        work, batch, baseline
                    );

                } else {

                    counts = Replay::wrapperBatch(
                        work, batch, wl
                    );
                }

                // Get the updated distances and parents
                const std::vector<long long>* newDist = nullptr;
                const std::vector<int>* newParent = nullptr;

                if (config.type == MethodType::Dijkstra) {
                    newDist = &dist;
                    newParent = &parent;

                } else if (config.type == MethodType::Baseline) {
                    newDist = &baseline.distances();
                    newParent = &baseline.parents();

                } else {
                    newDist = &wl.repairCore().distances();
                    newParent = &wl.repairCore().parents();
                }

                std::string outcome = "PASS";

                // Check the result
                if (counts.status != Status::Ok) {

                    outcome = "OVERFLOW";

                } else if (!Validator::validateOutput(
                               work, *newDist, *newParent, refQ)) {

                    outcome = "WRONG";

                } else if (config.type == MethodType::Baseline &&
                           !baseline.checkSupport(work)) {

                    outcome = "STRUCTURE_ERROR";

                } else if (config.type == MethodType::Wrapper &&
                           !wl.repairCore().checkSupport(work)) {

                    outcome = "STRUCTURE_ERROR";

                } else if (config.infinite) {

                    // If baseline failed we cannot compare counts
                    // Report COUNT_ERROR in that case
                    if (!baselineValid[b] ||
                        counts.total != baselineTotals[b]) {

                        outcome = "COUNT_ERROR";
                    }
                }

                // Save the baseline counts
                if (config.type == MethodType::Baseline &&
                    outcome == "PASS") {

                    baselineTotals[b] = counts.total;
                    baselineValid[b] = true;
                }

                // Overflow means no complete W_total
                const ReplayCounts* savedCounts =
                    (counts.status == Status::Ok) ? &counts : nullptr;

                writeRow(csv, info, batch, b, savedCounts, outcome);

                if (outcome != "PASS") {
                    writeNotRun(csv, info, sequence, b + 1);
                    finalOutcome = outcome;
                    allPassed = false;
                    break;
                }
            }

            // Print the result for this configuration
            std::cout << label
                      << " seed=" << seed
                      << " config=" << config.name
                      << " outcome=" << finalOutcome << '\n';

            std::cout.flush();
        }
    }

    csv.close();

    return allPassed ? 0 : 1;
}

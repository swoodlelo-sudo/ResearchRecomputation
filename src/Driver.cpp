#include "Graph.h"
#include "BatchGen.h"
#include "Dijkstra.h"
#include "DDFLPD.h"
#include "DDFLPD_WL.h"
#include "Validator.h"
#include "MinHeap.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <new>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <psapi.h>
#endif

using Clock = std::chrono::steady_clock;
using ULL = unsigned long long;

enum class MethodType { Dijkstra, Baseline, Wrapper };

struct Configuration {
    const char* name;
    MethodType type;
    int num;
    int den;
    bool infinite;
    const char* alpha;
};

const Configuration configs[] = {
    // Dijkstra and Baseline do not use alpha. Use legal placeholders.
    {"Dijkstra", MethodType::Dijkstra, 1, 1, false, "NA"},
    {"Baseline", MethodType::Baseline, 1, 1, false, "NA"},
    {"WL_1", MethodType::Wrapper, 1, 1, false, "1"},
    {"WL_075", MethodType::Wrapper, 3, 4, false, "0.75"},
    {"WL_05", MethodType::Wrapper, 1, 2, false, "0.5"},
    {"WL_025", MethodType::Wrapper, 1, 4, false, "0.25"},
    {"WL_inf", MethodType::Wrapper, 0, 1, true, "inf"}
};

const int graphSizes[] = {10000, 30000, 90000, 270000, 810000};
const int finalSeeds[] = {17401, 17402, 17403};
constexpr int batchCount = 30;
constexpr int configCount = 7;

int graphIndex(int N) {
    for (int i = 0; i < 5; ++i) {
        if (graphSizes[i] == N) return i;
    }
    return -1;
}

const char* batchPercent(int code) {
    if (code == 1) return "0.1";
    if (code == 10) return "1";
    if (code == 100) return "10";
    return "NA";
}

long long elapsedUs(Clock::time_point start, Clock::time_point end) {
    return std::chrono::duration_cast<std::chrono::microseconds>(
        end - start
    ).count();
}

struct ScheduleEntry {
    std::string label;
    int seed;
    int rep;
    int position;
    int configIndex;
};

std::vector<ScheduleEntry> makeSchedule(int g, bool pilot) {
    std::vector<ScheduleEntry> schedule;

    if (pilot) {
        for (int p = 0; p < configCount; ++p) {
            schedule.push_back({
                "PILOT", 17400, 0, p, (p + g) % configCount
            });
        }
        return schedule;
    }

    for (int j = 0; j < 3; ++j) {
        int seed = finalSeeds[j];

        // Warm-up uses the order for r = 0.
        int warmShift = (g + j) % configCount;
        for (int p = 0; p < configCount; ++p) {
            schedule.push_back({
                "WARMUP", seed, 0, p, (p + warmShift) % configCount
            });
        }

        for (int r = 0; r < 5; ++r) {
            int shift = (g + j + r) % configCount;
            for (int p = 0; p < configCount; ++p) {
                schedule.push_back({
                    "FINAL", seed, r, p, (p + shift) % configCount
                });
            }
        }
    }
    return schedule;
}

bool saveSchedule(const std::string& file,
                  const std::vector<ScheduleEntry>& schedule) {
    std::ofstream out(file);
    if (!out) return false;

    out << "label,seed,r,position,config\n";
    for (const auto& entry : schedule) {
        out << entry.label << ',' << entry.seed << ','
            << entry.rep << ',' << entry.position << ','
            << configs[entry.configIndex].name << '\n';
    }

    out.close();
    return !out.fail();
}

//out put of graph csv rows affected vrt and peak mem

struct ResultRow {
    std::string label;
    std::string graphFile;
    int n = 0;
    int m = 0;
    int seed = 0;
    int rep = 0;
    int runOrder = 0;
    std::string config;
    std::string alpha;

    int batchId = 0;
    std::string batchPct;
    std::size_t batchEdges = 0;
    long long initUs = 0;

    std::string wD = "NA";
    std::string wB = "NA";
    std::string limit = "NA";
    std::string wR = "NA";
    std::string wPreSwitch = "NA";
    std::string wRestart = "NA";
    std::string switched = "NA";

    long long changed = 0;
    long long affected = 0;
    ULL degreeSum = 0;
    long long timeUs = 0;
    std::string outcome = "NOT_RUN";
    std::string commit;
};

void writeHeader(std::ofstream& out) {
    out << "label,graph_file,source_original,n,m,seed,rep,"
        << "run_order,config,alpha,batch_id,batch_pct,"
        << "batch_edges,init_us,W_D,W_B,L,W_R,"
        << "W_pre_switch,W_restart,switched,N_changed,"
        << "N_affected,D_affected,time_us,time_ms,"
        << "outcome,commit\n";
}

void writeRow(std::ofstream& out, const ResultRow& x) {
    out << x.label << ',' << x.graphFile << ',' << 1 << ','
        << x.n << ',' << x.m << ',' << x.seed << ','
        << x.rep << ',' << x.runOrder << ','
        << x.config << ',' << x.alpha << ','
        << x.batchId << ',' << x.batchPct << ','
        << x.batchEdges << ',' << x.initUs << ','
        << x.wD << ',' << x.wB << ',' << x.limit << ','
        << x.wR << ',' << x.wPreSwitch << ','
        << x.wRestart << ',' << x.switched << ','
        << x.changed << ',' << x.affected << ','
        << x.degreeSum << ',' << x.timeUs << ','
        << std::fixed << std::setprecision(3)
        << x.timeUs / 1000.0 << ','
        << x.outcome << ',' << x.commit << '\n';

    out.flush();
}

void countAffected(const Graph& graph,
                   const std::vector<long long>& oldDist,
                   const std::vector<int>& oldParent,
                   const std::vector<long long>& newDist,
                   const std::vector<int>& newParent,
                   ResultRow& row) {
    for (int v = 0; v < graph.getVertexCount(); ++v) {
        bool changedDistance = (oldDist[v] != newDist[v]);
        bool changedParent = (oldParent[v] != newParent[v]);

        if (changedDistance) ++row.changed;
        if (changedDistance || changedParent) {
            ++row.affected;
            row.degreeSum += graph.degreeOf(v);
        }
    }
}

void writeNotRun(std::ofstream& out,
                 ResultRow row,
                 const BatchSequence& sequence,
                 int firstBatch) {
    for (int b = firstBatch; b < batchCount; ++b) {
        row.batchId = b;
        row.batchPct = batchPercent(sequence.batches[b].code);
        row.batchEdges = sequence.batches[b].updates.size();

        row.wR = "NA";
        row.wPreSwitch = "NA";
        row.wRestart = "NA";
        row.switched = "NA";
        row.changed = 0;
        row.affected = 0;
        row.degreeSum = 0;
        row.timeUs = 0;
        row.outcome = "NOT_RUN";

        writeRow(out, row);
    }
}

bool checkOldWeights(const Graph& graph, const Batch& batch) {
    for (const auto& update : batch.updates) {
        if (graph.getWeight(update.edgeId) != update.oldWeight)
            return false;
    }
    return true;
}

unsigned long long peakMemoryBytes() {
#ifdef _WIN32
    PROCESS_MEMORY_COUNTERS info{};
    if (GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info))) {
        return static_cast<ULL>(info.PeakWorkingSetSize);
    }
#endif
    return 0;
}
//run a sequence from whss listed

std::string runSequence(const Graph& original,
                        const BatchSequence& sequence,
                        const ScheduleEntry& entry,
                        const std::string& graphFile,
                        const std::string& commit,
                        std::ofstream& csv) {
    const Configuration& config = configs[entry.configIndex];

    ResultRow row;
    row.label = entry.label;
    row.graphFile = graphFile;
    row.n = original.getVertexCount();
    row.m = original.getEdgeCount();
    row.seed = entry.seed;
    row.rep = entry.rep;
    row.runOrder = entry.position;
    row.config = config.name;
    row.alpha = config.alpha;
    row.commit = commit;

    // Include initialization and graph copying in the timeout.
    auto sequenceStart = Clock::now();
    int nextBatch = 0;

    try {
        Graph work = original;

        DDFLPD baseline;
        DDFLPD_WL wl(config.num, config.den, config.infinite);
        MinHeap Q;
        MinHeap refQ;
        std::vector<long long> dist;
        std::vector<int> parent;
        ULL ignoredWork = 0;
        Status initStatus = Status::Ok;

        // Initialization is timed separately.
        auto initStart = Clock::now();

        if (config.type == MethodType::Dijkstra) {
            Q.setup(work.getVertexCount());
            initStatus = Dijkstra::run(
                work, Q, dist, parent, ignoredWork, false
            );
        } else if (config.type == MethodType::Baseline) {
            initStatus = baseline.initialize(work);
        } else {
            initStatus = wl.initialize(work);
        }

        auto initEnd = Clock::now();
        row.initUs = elapsedUs(initStart, initEnd);

        if (initStatus != Status::Ok) {
            // Batch 0 reports the failure; batches 1-29 are NOT_RUN.
            row.batchId = 0;
            row.batchPct = batchPercent(sequence.batches[0].code);
            row.batchEdges = sequence.batches[0].updates.size();
            row.outcome = "OVERFLOW";
            writeRow(csv, row);
            writeNotRun(csv, row, sequence, 1);
            return "OVERFLOW";
        }

        // Used only for validation, outside the timer.
        refQ.setup(work.getVertexCount());

        for (int b = 0; b < batchCount; ++b) {
            nextBatch = b;
            const Batch& batch = sequence.batches[b];

            row.batchId = b;
            row.batchPct = batchPercent(batch.code);
            row.batchEdges = batch.updates.size();
            row.changed = 0;
            row.affected = 0;
            row.degreeSum = 0;
            row.wD = "NA";
            row.wB = "NA";
            row.limit = "NA";
            row.wR = "NA";
            row.wPreSwitch = "NA";
            row.wRestart = "NA";
            row.switched = "NA";

            if (!checkOldWeights(work, batch)) {
                std::cerr << "Old weight mismatch: N=" << row.n
                          << " seed=" << entry.seed
                          << " config=" << config.name
                          << " batch=" << b << '\n';
                return "DRIVER_ERROR";
            }

            // Save the current tree before the update.
            std::vector<long long> oldDist;
            std::vector<int> oldParent;

            if (config.type == MethodType::Dijkstra) {
                oldDist = dist;
                oldParent = parent;
            } else if (config.type == MethodType::Baseline) {
                oldDist = baseline.distances();
                oldParent = baseline.parents();
            } else {
                oldDist = wl.repairCore().distances();
                oldParent = wl.repairCore().parents();
            }

            Status dijkstraStatus = Status::Ok;
            RepairResult repairStatus = RepairResult::Finished;
            WLOutcome outcome{};

            // Only the update belongs inside this timer.
            auto start = Clock::now();

            if (config.type == MethodType::Dijkstra) {
                for (const auto& update : batch.updates) {
                    work.setWeight(update.edgeId, update.newWeight);
                }
                dijkstraStatus = Dijkstra::run(
                    work, Q, dist, parent, ignoredWork, false
                );
            } else if (config.type == MethodType::Baseline) {
                repairStatus = baseline.repair(work, batch, false, 0);
            } else {
                outcome = wl.update(work, batch);
            }

            auto end = Clock::now();
            row.timeUs = elapsedUs(start, end);

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
                row.wD = std::to_string(wl.getWD());
                row.wB = std::to_string(wl.getWB());
                row.limit = std::to_string(wl.getL());
                row.wR = std::to_string(outcome.wR);
                row.wPreSwitch = std::to_string(outcome.wPreSwitch);
                row.wRestart = std::to_string(outcome.wRestart);
                row.switched = outcome.switched ? "yes" : "no";
            }

            bool overflow =
                (config.type == MethodType::Dijkstra &&
                 dijkstraStatus == Status::Overflow) ||
                (config.type == MethodType::Baseline &&
                 repairStatus == RepairResult::Overflow) ||
                (config.type == MethodType::Wrapper &&
                 outcome.status == Status::Overflow);

            if (overflow) {
                row.outcome = "OVERFLOW";
            } else if (!Validator::validateOutput(
                           work, *newDist, *newParent, refQ)) {
                row.outcome = "WRONG";
            } else if (config.type == MethodType::Baseline &&
                       !baseline.checkSupport(work)) {
                row.outcome = "STRUCTURE_ERROR";
            } else if (config.type == MethodType::Wrapper &&
                       !wl.repairCore().checkSupport(work)) {
                row.outcome = "STRUCTURE_ERROR";
            } else if (config.type == MethodType::Baseline &&
                       baseline.lastWR() != 0) {
                row.outcome = "COUNT_ERROR";
            } else if (config.type == MethodType::Wrapper) {
                bool badCount = false;

                if (outcome.switched) {
                    if (config.infinite ||
                        wl.getL() == std::numeric_limits<ULL>::max() ||
                        outcome.wPreSwitch != wl.getL() + 1) {
                        badCount = true;
                    }
                } else if (outcome.wR > wl.getL() ||
                           outcome.wPreSwitch != 0 ||
                           outcome.wRestart != 0) {
                    badCount = true;
                }

                row.outcome = badCount ? "COUNT_ERROR" : "PASS";
            } else {
                row.outcome = "PASS";
            }

            // Invalid output may not have usable vectors.
            if (row.outcome == "PASS") {
                countAffected(work, oldDist, oldParent,
                              *newDist, *newParent, row);
            }

            // Check the full-sequence timeout between batches.
            double seconds = std::chrono::duration<double>(
                Clock::now() - sequenceStart
            ).count();
            if (row.outcome == "PASS" && seconds > 600.0) {
                row.outcome = "TIMEOUT";
            }

            writeRow(csv, row);
            nextBatch = b + 1;

            if (row.outcome != "PASS") {
                writeNotRun(csv, row, sequence, nextBatch);
                return row.outcome;
            }
        }
        return "PASS";

    } catch (const std::bad_alloc&) {
        // Keep one row for the failed batch, then skip the rest.
        if (nextBatch < batchCount) {
            row.batchId = nextBatch;
            row.batchPct = batchPercent(sequence.batches[nextBatch].code);
            row.batchEdges = sequence.batches[nextBatch].updates.size();
            row.changed = 0;
            row.affected = 0;
            row.degreeSum = 0;
            row.timeUs = 0;
            row.outcome = "OUT_OF_MEMORY";
            writeRow(csv, row);
            writeNotRun(csv, row, sequence, nextBatch + 1);
        }
        return "OUT_OF_MEMORY";
    }
}

// main 

int main(int argc, char* argv[]) {
    if (argc != 4) {
        std::cerr << "Usage: ./build/Driver.exe <N> <pilot|final> <commit>\n";
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
        std::cerr << "Usage: ./build/Driver.exe <N> <pilot|final> <commit>\n";
        return 1;
    }

    int g = graphIndex(N);
    std::string mode = argv[2];
    std::string commit = argv[3];

    if (g == -1 || (mode != "pilot" && mode != "final") ||
        commit.size() != 7) {
        std::cerr << "Usage: ./build/Driver.exe <N> <pilot|final> <commit>\n";
        return 1;
    }

    std::filesystem::create_directories("Results");

    const std::string stem = "CAL_" + std::to_string(N);
    const std::string graphFile = stem + ".txt";

    Graph original;
    if (!original.load("data/graphs/" + graphFile)) {
        std::cerr << "Could not load " << graphFile << '\n';
        return 1;
    }
    if (original.getVertexCount() != N) {
        std::cerr << "Graph size mismatch.\n";
        return 1;
    }

    std::vector<int> seeds;
    if (mode == "pilot") {
        seeds = {17400};
    } else {
        seeds = {17401, 17402, 17403};
    }

    // Check every batch file before starting.
    std::vector<BatchSequence> sequences;
    for (int seed : seeds) {
        BatchSequence sequence;
        const std::string file = "data/batches/" + stem +
            "_seed" + std::to_string(seed) + ".txt";

        if (!BatchGenerator::load(file, sequence)) {
            std::cerr << "Could not load " << file << '\n';
            return 1;
        }
        if (sequence.edgeCount != original.getEdgeCount() ||
            sequence.seed != static_cast<ULL>(seed) ||
            sequence.batches.size() != batchCount) {
            std::cerr << "Batch sequence mismatch: " << file << '\n';
            return 1;
        }
        sequences.push_back(std::move(sequence));
    }

    // Save the schedule before timing anything.
    auto schedule = makeSchedule(g, mode == "pilot");
    const std::string scheduleFile =
        "Results/schedule_" + stem + "_" + mode + ".csv";
    if (!saveSchedule(scheduleFile, schedule)) {
        std::cerr << "Could not save schedule.\n";
        return 1;
    }

    const std::string csvFile =
        "Results/timed_" + stem + "_" + mode + ".csv";
    std::ofstream csv(csvFile);
    if (!csv) {
        std::cerr << "Could not open results CSV.\n";
        return 1;
    }
    writeHeader(csv);
    csv.flush();

    const std::string infoFile =
        "Results/runinfo_" + stem + "_" + mode + ".txt";
    std::ofstream info(infoFile);
    if (!info) {
        std::cerr << "Could not open run-info file.\n";
        return 1;
    }

    info << "Compiler: " << __VERSION__ << '\n';
    info << "Flags: -std=c++17 -O2 -Wall -Wextra\n";
    info << "Commit: " << commit << '\n';
    info << "N: " << N << '\n';
    info << "Mode: " << mode << '\n';
    info << "Graph: " << graphFile << '\n';
    info.flush();

    bool allPassed = true;

    for (const auto& entry : schedule) {
        const BatchSequence* sequence = nullptr;
        for (const auto& candidate : sequences) {
            if (candidate.seed == static_cast<ULL>(entry.seed)) {
                sequence = &candidate;
                break;
            }
        }

        if (sequence == nullptr) {
            std::cerr << "Missing seed " << entry.seed << '\n';
            return 1;
        }

        auto start = Clock::now();
        std::string result = runSequence(
            original, *sequence, entry, graphFile, commit, csv
        );
        auto end = Clock::now();

        double seconds =
            std::chrono::duration<double>(end - start).count();
        std::cout << entry.label
                  << " seed=" << entry.seed
                  << " r=" << entry.rep
                  << " config=" << configs[entry.configIndex].name
                  << " seconds=" << std::fixed << std::setprecision(3)
                  << seconds << " outcome=" << result << '\n';
        std::cout.flush();

        if (result == "DRIVER_ERROR") return 1;
        if (result != "PASS") allPassed = false;
    }

    ULL peak = peakMemoryBytes();
    info << "PeakWorkingSetBytes: " << peak << '\n';
    info << "PeakWorkingSetGiB: " << std::fixed << std::setprecision(3)
         << peak / (1024.0 * 1024.0 * 1024.0) << '\n';

    info.close();
    csv.close();
    return allPassed ? 0 : 1;
}

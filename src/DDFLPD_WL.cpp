// This file has the functions declared in DDFLPD_WL.h.
// All repair work stays in DDFLPD, so the baseline and wrapper use the same code.

#include "DDFLPD_WL.h"

#include <cassert>
#include <limits>

DDFLPD_WL::DDFLPD_WL(int num, int den, bool infinite)
    : alphaNum(num), alphaDen(den), alphaInfinite(infinite) {
    assert(alphaInfinite || (alphaNum > 0 && alphaDen > 0));
}

bool DDFLPD_WL::computeLimit(unsigned long long S, int num, int den,
                            unsigned long long& limit) {
    if (num <= 0 || den <= 0) return false;

    // Cast before multiplying so the intermediate product cannot wrap at 64 bits.
    const unsigned __int128 product =
        static_cast<unsigned __int128>(S) * static_cast<unsigned int>(num);

    const unsigned __int128 result =
        product / static_cast<unsigned int>(den);

    if (result > std::numeric_limits<unsigned long long>::max()) {
        return false;
    }

    limit = static_cast<unsigned long long>(result);
    return true;
}

Status DDFLPD_WL::initialize(const Graph& graph) {
    const Status status = core.initialize(graph);
    if (status != Status::Ok) return status;

    const unsigned long long maximum =
        std::numeric_limits<unsigned long long>::max();

    const unsigned long long wD = core.getWD();
    const unsigned long long wB = core.getWB();

    if (wB > maximum - wD) return Status::Overflow;
    const unsigned long long S = wD + wB;

    if (alphaInfinite) {
        L = maximum;
    } else if (!computeLimit(S, alphaNum, alphaDen, L)) {
        return Status::Overflow;
    }

    return Status::Ok;
}

WLOutcome DDFLPD_WL::update(Graph& graph, const Batch& batch) {
    WLOutcome outcome;

    // Counting stays on even when alpha is infinity.
    const RepairResult result = core.repair(graph, batch, true, L);
    outcome.wR = core.lastWR();

    if (result == RepairResult::Finished) {
        return outcome;
    }

    if (result == RepairResult::LimitExceeded) {
        outcome.wPreSwitch = outcome.wR;

        core.abandonRepair();                         // Line 18

        unsigned long long restartWork = 0;
        outcome.status = core.rebuild(graph, restartWork); // Lines 19-21

        if (outcome.status != Status::Ok) return outcome;

        outcome.wRestart = restartWork;
        outcome.switched = true;
        return outcome;
    }

    // Overflow: empty Q and restore white colors. This does not repair the tree.
    // Keep wR so the caller can see how much work happened before the stop.
    core.abandonRepair();
    outcome.status = Status::Overflow;
    return outcome;
}

unsigned long long DDFLPD_WL::getL() const {
    return L;
}

unsigned long long DDFLPD_WL::getWD() const {
    return core.getWD();
}

unsigned long long DDFLPD_WL::getWB() const {
    return core.getWB();
}

const DDFLPD& DDFLPD_WL::repairCore() const {
    return core;
}
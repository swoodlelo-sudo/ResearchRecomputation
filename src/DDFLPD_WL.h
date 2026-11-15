#ifndef DDFLPD_WL_H
#define DDFLPD_WL_H

#include "DDFLPD.h"

// The numbers for one batch. A failed run's distances and parents must not
// be used. switched is true once a fallback finishes successfully.
struct WLOutcome {
    Status status = Status::Ok;
    bool switched = false;
    unsigned long long wR = 0;
    unsigned long long wPreSwitch = 0;
    unsigned long long wRestart = 0;
};

// This wraps the same DDFLPD repair the baseline uses. Counting is always on.
// The initial W_D, W_B and limit stay fixed for the whole batch sequence.
class DDFLPD_WL {
public:
    // For example: (3, 4, false) is alpha = 0.75.
    // (0, 1, true) is infinity: count work but never switch.
    DDFLPD_WL(int num, int den, bool infinite);

    // Exact floor(S * num / den), using a wider integer for the product.
    // Returns false for a non-positive fraction or an oversized result.
    // On failure, the output limit is unchanged.
    [[nodiscard]] static bool computeLimit(
        unsigned long long S,
        int num,
        int den,
        unsigned long long& limit);

    // Algorithm 0: initialize the core and compute L once for this sequence.
    [[nodiscard]] Status initialize(const Graph& graph);

    // Algorithm 3: repair, then abandon and rebuild if the limit is exceeded.
    // initialize must have succeeded first. On Overflow, the tree is invalid;
    // stop this sequence and initialize again before another update.
    [[nodiscard]] WLOutcome update(Graph& graph, const Batch& batch);

    unsigned long long getL() const;
    unsigned long long getWD() const;
    unsigned long long getWB() const;
    const DDFLPD& repairCore() const;

private:
    DDFLPD core;
    int alphaNum;
    int alphaDen;
    bool alphaInfinite;
    unsigned long long L = 0;
};

#endif
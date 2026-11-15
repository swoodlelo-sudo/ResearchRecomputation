#ifndef DDFLPD_WL_STATUS_H
#define DDFLPD_WL_STATUS_H

#include <limits>

// Shared result for operations that can overflow.
enum class Status {
    Ok,
    Overflow
};

// Add one unit of work only when it fits. On failure, leave work unchanged.
// This short helper is inline so multiple source files can include it.
[[nodiscard]] inline bool checkedIncrement(unsigned long long& work) {
    if (work == std::numeric_limits<unsigned long long>::max()) {
        return false;
    }

    work += 1;
    return true;
}

#endif
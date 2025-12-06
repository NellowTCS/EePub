#include "MemoryMonitor.h"

#include <utility>

MemoryMonitor& MemoryMonitor::instance() {
    static MemoryMonitor monitor;
    return monitor;
}

void MemoryMonitor::setSoftLimit(std::size_t bytes, std::function<void(std::size_t)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    softLimitBytes_ = bytes;
    callback_ = std::move(callback);
}

void MemoryMonitor::updateVirtualUsage(std::size_t bytes) {
    std::function<void(std::size_t)> callbackCopy;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        currentBytes_ = bytes;
        if (bytes > peakBytes_) {
            peakBytes_ = bytes;
        }
        if (softLimitBytes_ > 0 && bytes > softLimitBytes_ && callback_) {
            callbackCopy = callback_;
        }
    }

    if (callbackCopy) {
        callbackCopy(bytes);
    }
}

EePubMemoryStats MemoryMonitor::snapshot() const {
    std::lock_guard<std::mutex> lock(mutex_);
    EePubMemoryStats stats;
    stats.currentBytes = currentBytes_;
    stats.peakBytes = peakBytes_;
    return stats;
}

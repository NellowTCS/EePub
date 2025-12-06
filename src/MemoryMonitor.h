#pragma once

#include <cstddef>
#include <functional>
#include <mutex>

struct EePubMemoryStats {
    std::size_t currentBytes = 0;
    std::size_t peakBytes = 0;
};

class MemoryMonitor {
public:
    static MemoryMonitor& instance();

    void setSoftLimit(std::size_t bytes, std::function<void(std::size_t)> callback);
    void updateVirtualUsage(std::size_t bytes);
    EePubMemoryStats snapshot() const;

private:
    MemoryMonitor() = default;
    MemoryMonitor(const MemoryMonitor&) = delete;
    MemoryMonitor& operator=(const MemoryMonitor&) = delete;

    mutable std::mutex mutex_;
    std::size_t currentBytes_ = 0;
    std::size_t peakBytes_ = 0;
    std::size_t softLimitBytes_ = 0;
    std::function<void(std::size_t)> callback_ = nullptr;
};

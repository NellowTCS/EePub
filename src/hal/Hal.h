#pragma once

#include <cstddef>
#include <cstdint>

enum class EePubLogLevel : std::uint8_t {
    Info,
    Warn,
    Error
};

struct EePubHal {
    void* userContext = nullptr;
    void (*log)(EePubLogLevel level, const char* message, void* userContext) = nullptr;
    std::size_t (*heapFreeBytes)(void* userContext) = nullptr;
    std::uint64_t (*uptimeMillis)(void* userContext) = nullptr;
};

void EePubHalInstall(const EePubHal* hal);
const EePubHal* EePubHalCurrent();
void EePubHalLog(EePubLogLevel level, const char* message);
std::size_t EePubHalHeapFreeBytes();
std::uint64_t EePubHalMillis();

#include "hal/Hal.h"

#include <mutex>

namespace {
std::mutex& halMutex() {
    static std::mutex mtx;
    return mtx;
}

const EePubHal*& halInstance() {
    static const EePubHal* instance = nullptr;
    return instance;
}
}

void EePubHalInstall(const EePubHal* hal) {
    std::lock_guard<std::mutex> lock(halMutex());
    halInstance() = hal;
}

const EePubHal* EePubHalCurrent() {
    std::lock_guard<std::mutex> lock(halMutex());
    return halInstance();
}

void EePubHalLog(EePubLogLevel level, const char* message) {
    const EePubHal* hal = EePubHalCurrent();
    if (hal && hal->log) {
        hal->log(level, message, hal->userContext);
    }
}

std::size_t EePubHalHeapFreeBytes() {
    const EePubHal* hal = EePubHalCurrent();
    if (hal && hal->heapFreeBytes) {
        return hal->heapFreeBytes(hal->userContext);
    }
    return 0;
}

std::uint64_t EePubHalMillis() {
    const EePubHal* hal = EePubHalCurrent();
    if (hal && hal->uptimeMillis) {
        return hal->uptimeMillis(hal->userContext);
    }
    return 0;
}

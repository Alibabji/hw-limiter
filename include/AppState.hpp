#pragma once

#include <string>
#include <vector>
#include <optional>

#include "HardwareInfo.hpp"
#include "ProfileLoader.hpp"
#include "ProfileEngine.hpp"
#include "PowerThrottler.hpp"

struct AppState {
    HardwareSnapshot snapshot;
    ProfileDatabase profiles;
    ProfileEngine engine;
    PowerThrottler throttler;

    std::vector<CpuThrottleTarget> cpuOptions;
    std::vector<GpuThrottleTarget> gpuOptions;

    std::optional<CpuThrottleTarget> selectedCpu;
    std::optional<GpuThrottleTarget> selectedGpu;

    bool initialized = false;
};

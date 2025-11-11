#pragma once

#include <optional>
#include <string>
#include <vector>

#include "HardwareInfo.hpp"
#include "ProfileLoader.hpp"
#include "ProfileEngine.hpp"
#include "PowerThrottler.hpp"
#include "BenchmarkTypes.hpp"

struct AppState {
    HardwareSnapshot snapshot;
    ProfileDatabase profiles;
    ProfileEngine engine;
    PowerThrottler throttler;

    std::vector<CpuThrottleTarget> cpuOptions;
    std::vector<GpuThrottleTarget> gpuOptions;

    std::optional<CpuThrottleTarget> selectedCpu;
    std::optional<GpuThrottleTarget> selectedGpu;

    BenchmarkSnapshot benchmark;
    double cpuNominalFrequencyMHz = 0.0;
    double gpuNominalClockMHz = 0.0;
    double gpuNominalPowerWatts = 0.0;

    bool initialized = false;
};

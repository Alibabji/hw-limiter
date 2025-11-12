#pragma once

#include <optional>
#include <string>

#include "HardwareInfo.hpp"
#include "BenchmarkTypes.hpp"

struct BenchmarkReport {
    std::optional<BenchmarkResultData> cpu;
    std::optional<BenchmarkResultData> gpu;
};

class BenchmarkRunner {
public:
    BenchmarkReport Run(const HardwareSnapshot& snapshot) const;

private:
    std::optional<BenchmarkResultData> RunCpuBenchmark(const HardwareSnapshot& snapshot) const;
    std::optional<BenchmarkResultData> RunGpuBenchmark(const HardwareSnapshot& snapshot) const;
};

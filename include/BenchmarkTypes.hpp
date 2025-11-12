#pragma once

#include <optional>
#include <string>

struct BenchmarkResultData {
    double score = 0.0;
    std::string unit;
    std::string details;
};

struct BenchmarkSnapshot {
    std::optional<BenchmarkResultData> baselineCpu;
    std::optional<BenchmarkResultData> baselineGpu;
    std::optional<BenchmarkResultData> currentCpu;
    std::optional<BenchmarkResultData> currentGpu;
};

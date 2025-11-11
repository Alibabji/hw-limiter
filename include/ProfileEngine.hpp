#pragma once

#include <string>
#include <vector>

#include "HardwareInfo.hpp"
#include "ProfileLoader.hpp"

class ProfileEngine {
public:
    ProfileEngine() = default;

    void Refresh(const HardwareSnapshot& snapshot, const ProfileDatabase& database);

    const std::vector<CpuThrottleTarget>& CpuOptions() const { return cpuOptions_; }
    const std::vector<GpuThrottleTarget>& GpuOptions() const { return gpuOptions_; }

private:
    static bool MatchesTokens(const std::string& haystack, const std::vector<std::string>& tokens);

    std::vector<CpuThrottleTarget> cpuOptions_;
    std::vector<GpuThrottleTarget> gpuOptions_;
};

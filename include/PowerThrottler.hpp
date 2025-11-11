#pragma once

#include <optional>
#include <string>

#include "ProfileLoader.hpp"

struct ThrottleResult {
    bool success = false;
    std::wstring message;
};

class PowerThrottler {
public:
    PowerThrottler() = default;

    ThrottleResult ApplyCpuTarget(const CpuThrottleTarget& target);
    ThrottleResult ApplyGpuTarget(const GpuThrottleTarget& target);
    ThrottleResult RestoreDefaults();

private:
    ThrottleResult RunCommand(const std::wstring& commandLine) const;
};

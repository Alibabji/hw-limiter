#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "HardwareInfo.hpp"
#include "SimpleJson.hpp"

struct CpuThrottleTarget {
    std::string id;
    std::string label;
    int maxFrequencyMHz = 0;
    int maxCores = 0;
    int maxThreads = 0;
    int maxPercent = 100;
    std::vector<std::string> extraCommands;  // optional shell commands
};

struct CpuProfile {
    std::string id;
    std::string label;
    std::vector<std::string> matchTokens;
    std::vector<CpuThrottleTarget> targets;
};

struct GpuThrottleTarget {
    std::string id;
    std::string label;
    int maxFrequencyMHz = 0;
    int powerLimitWatts = 0;
    std::vector<std::string> nvidiaSmiArgs;
};

struct GpuProfile {
    std::string id;
    std::string label;
    std::vector<std::string> matchTokens;
    std::vector<GpuThrottleTarget> targets;
};

struct ProfileDatabase {
    std::vector<CpuProfile> cpuProfiles;
    std::vector<GpuProfile> gpuProfiles;
};

class ProfileLoader {
public:
    ProfileDatabase LoadFromFile(const std::filesystem::path& path) const;

private:
    CpuProfile ParseCpuProfile(const jsonlite::Value& value) const;
    GpuProfile ParseGpuProfile(const jsonlite::Value& value) const;
    std::vector<std::string> ParseStringArray(const jsonlite::Value& value) const;
};

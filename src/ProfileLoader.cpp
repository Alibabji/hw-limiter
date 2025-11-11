#include "ProfileLoader.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <stdexcept>

using jsonlite::Value;

namespace {

std::string ReadUtf8File(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) {
        throw std::runtime_error("Unable to open profile file");
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

}  // namespace

ProfileDatabase ProfileLoader::LoadFromFile(const std::filesystem::path& path) const {
    auto text = ReadUtf8File(path);
    auto root = jsonlite::Parse(text);

    ProfileDatabase db;
    const Value& cpuProfiles = root["cpuProfiles"];
    if (cpuProfiles.IsArray()) {
        for (const auto& entry : cpuProfiles.array) {
            db.cpuProfiles.push_back(ParseCpuProfile(entry));
        }
    }

    const Value& gpuProfiles = root["gpuProfiles"];
    if (gpuProfiles.IsArray()) {
        for (const auto& entry : gpuProfiles.array) {
            db.gpuProfiles.push_back(ParseGpuProfile(entry));
        }
    }
    return db;
}

CpuProfile ProfileLoader::ParseCpuProfile(const Value& value) const {
    CpuProfile profile;
    profile.id = value["id"].GetString();
    profile.label = value["label"].GetString();
    profile.matchTokens = ParseStringArray(value["matchTokens"]);

    const Value& targets = value["targets"];
    if (targets.IsArray()) {
        for (const auto& entry : targets.array) {
            CpuThrottleTarget target;
            target.id = entry["id"].GetString();
            target.label = entry["label"].GetString();
            target.maxFrequencyMHz = static_cast<int>(entry["maxFrequencyMHz"].GetNumber(0));
            target.maxCores = static_cast<int>(entry["maxCores"].GetNumber(0));
            target.maxThreads = static_cast<int>(entry["maxThreads"].GetNumber(0));
            target.maxPercent = static_cast<int>(entry["maxPercent"].GetNumber(100));
            target.extraCommands = ParseStringArray(entry["extraCommands"]);
            target.requiresConfirmation = entry["requiresConfirmation"].GetBool(false);
            profile.targets.push_back(std::move(target));
        }
    }
    return profile;
}

GpuProfile ProfileLoader::ParseGpuProfile(const Value& value) const {
    GpuProfile profile;
    profile.id = value["id"].GetString();
    profile.label = value["label"].GetString();
    profile.matchTokens = ParseStringArray(value["matchTokens"]);

    const Value& targets = value["targets"];
    if (targets.IsArray()) {
        for (const auto& entry : targets.array) {
            GpuThrottleTarget target;
            target.id = entry["id"].GetString();
            target.label = entry["label"].GetString();
            target.maxFrequencyMHz = static_cast<int>(entry["maxFrequencyMHz"].GetNumber(0));
            target.powerLimitWatts = static_cast<int>(entry["powerLimitWatts"].GetNumber(0));
            target.nvidiaSmiArgs = ParseStringArray(entry["nvidiaSmiArgs"]);
            target.requiresConfirmation = entry["requiresConfirmation"].GetBool(false);
            profile.targets.push_back(std::move(target));
        }
    }
    return profile;
}

std::vector<std::string> ProfileLoader::ParseStringArray(const Value& value) const {
    std::vector<std::string> items;
    if (!value.IsArray()) {
        return items;
    }
    for (const auto& entry : value.array) {
        if (entry.IsString()) {
            items.push_back(entry.string);
        }
    }
    return items;
}

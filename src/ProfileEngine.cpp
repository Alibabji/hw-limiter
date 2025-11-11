#include "ProfileEngine.hpp"

#include <algorithm>
#include <cctype>
#include <string>

namespace {

std::string ToLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

}  // namespace

void ProfileEngine::Refresh(const HardwareSnapshot& snapshot, const ProfileDatabase& database) {
    cpuOptions_.clear();
    gpuOptions_.clear();
    cpuNominalFrequencyMHz_ = 0;
    gpuNominalFrequencyMHz_ = 0;
    gpuNominalPowerWatts_ = 0;

    const std::string cpuNameLower = ToLower(snapshot.cpu.name);
    for (const auto& profile : database.cpuProfiles) {
        if (MatchesTokens(cpuNameLower, profile.matchTokens)) {
            cpuOptions_.insert(cpuOptions_.end(), profile.targets.begin(), profile.targets.end());
            if (cpuNominalFrequencyMHz_ == 0 && profile.nominalFrequencyMHz > 0) {
                cpuNominalFrequencyMHz_ = profile.nominalFrequencyMHz;
            }
        }
    }

    if (!snapshot.gpus.empty()) {
        std::wstring gpuNameWide = snapshot.gpus.front().name;
        std::string gpuName(gpuNameWide.begin(), gpuNameWide.end());
        std::string gpuLower = ToLower(gpuName);
        for (const auto& profile : database.gpuProfiles) {
            if (MatchesTokens(gpuLower, profile.matchTokens)) {
                gpuOptions_.insert(gpuOptions_.end(), profile.targets.begin(), profile.targets.end());
                if (gpuNominalFrequencyMHz_ == 0 && profile.nominalFrequencyMHz > 0) {
                    gpuNominalFrequencyMHz_ = profile.nominalFrequencyMHz;
                }
                if (gpuNominalPowerWatts_ == 0 && profile.nominalPowerWatts > 0) {
                    gpuNominalPowerWatts_ = profile.nominalPowerWatts;
                }
            }
        }
    }
}

bool ProfileEngine::MatchesTokens(const std::string& haystack, const std::vector<std::string>& tokens) {
    if (tokens.empty()) {
        return false;
    }
    for (const auto& token : tokens) {
        std::string tokenLower = ToLower(token);
        if (!tokenLower.empty() && haystack.find(tokenLower) != std::string::npos) {
            return true;
        }
    }
    return false;
}

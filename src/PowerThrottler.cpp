#include "PowerThrottler.hpp"

#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <Windows.h>
#endif

namespace {

#ifdef _WIN32
std::wstring BuildCommand(const std::wstring& exe, const std::wstring& args) {
    std::wstringstream ss;
    ss << exe;
    if (!args.empty()) {
        ss << L" " << args;
    }
    return ss.str();
}

std::wstring ToWide(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}
#endif

}  // namespace

ThrottleResult PowerThrottler::ApplyCpuTarget(const CpuThrottleTarget& target) {
#ifdef _WIN32
    auto maxPercent = target.maxPercent > 0 ? target.maxPercent : 100;
    std::vector<std::wstring> commands;
    auto percentStr = std::to_wstring(maxPercent);
    commands.push_back(L"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX " + percentStr);
    commands.push_back(L"powercfg /setdcvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX " + percentStr);
    commands.push_back(L"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMIN " + percentStr);
    commands.push_back(L"powercfg /setdcvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMIN " + percentStr);
    commands.push_back(L"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 3");
    commands.push_back(L"powercfg /setdcvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 3");

    if (target.maxFrequencyMHz > 0) {
        auto freqStr = std::to_wstring(target.maxFrequencyMHz);
        commands.push_back(L"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCFREQMAX " + freqStr);
        commands.push_back(L"powercfg /setdcvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCFREQMAX " + freqStr);
    }

    for (const auto& extra : target.extraCommands) {
        commands.push_back(ToWide(extra));
    }
    commands.push_back(L"powercfg /setactive SCHEME_CURRENT");

    ThrottleResult result;
    result.success = true;
    for (const auto& cmd : commands) {
        auto step = RunCommand(cmd);
        if (!step.success) {
            return step;
        }
    }
    result.message = L"CPU target applied";
    return result;
#else
    return {false, L"Power throttling is only supported on Windows"};
#endif
}

ThrottleResult PowerThrottler::ApplyGpuTarget(const GpuThrottleTarget& target) {
#ifdef _WIN32
    if (target.nvidiaSmiArgs.empty()) {
        return {false, L"No GPU commands defined for this target"};
    }
    std::vector<std::wstring> commands;
    commands.push_back(L"nvidia-smi -i 0 -pm 1");
    std::wstring args = L"-i 0";
    for (const auto& part : target.nvidiaSmiArgs) {
        args += L" ";
        args += ToWide(part);
    }
    commands.push_back(L"nvidia-smi " + args);

    for (const auto& cmd : commands) {
        auto step = RunCommand(cmd);
        if (!step.success) {
            return step;
        }
    }
    return {true, L"GPU target applied"};
#else
    return {false, L"GPU throttling is only supported on Windows"};
#endif
}

ThrottleResult PowerThrottler::RestoreDefaults() {
#ifdef _WIN32
    ThrottleResult result = RunCommand(L"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX 100");
    if (!result.success) {
        return result;
    }
    result = RunCommand(L"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCFREQMAX 0");
    if (!result.success) {
        return result;
    }
    result.message = L"Default power limits restored";
    return result;
#else
    return {false, L"Restore is only supported on Windows"};
#endif
}

ThrottleResult PowerThrottler::RunCommand(const std::wstring& commandLine) const {
#ifdef _WIN32
    std::wstring fullCommand = L"cmd.exe /C " + commandLine;
    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    std::wstring mutableCommand = fullCommand;
    if (!CreateProcessW(nullptr, mutableCommand.data(), nullptr, nullptr, FALSE,
                        CREATE_NO_WINDOW, nullptr, nullptr, &si, &pi)) {
        DWORD error = GetLastError();
        wchar_t buffer[256];
        swprintf_s(buffer, L"Failed to execute '%s' (error %lu)", commandLine.c_str(), error);
        return {false, buffer};
    }

    WaitForSingleObject(pi.hProcess, INFINITE);
    DWORD exitCode = 1;
    GetExitCodeProcess(pi.hProcess, &exitCode);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (exitCode != 0) {
        wchar_t buffer[256];
        swprintf_s(buffer, L"Command '%s' exited with %lu", commandLine.c_str(), exitCode);
        return {false, buffer};
    }
    return {true, L"OK"};
#else
    (void)commandLine;
    return {false, L"Commands only run on Windows"};
#endif
}

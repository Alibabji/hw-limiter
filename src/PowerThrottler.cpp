#include "PowerThrottler.hpp"

#include <sstream>
#include <string>

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
    auto cmdMax = L"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCTHROTTLEMAX " +
                  std::to_wstring(maxPercent);
    auto result = RunCommand(cmdMax);
    if (!result.success) {
        return result;
    }

    std::wstring freqCmd;
    if (target.maxFrequencyMHz > 0) {
        freqCmd = L"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PROCFREQMAX " +
                  std::to_wstring(target.maxFrequencyMHz);
        result = RunCommand(freqCmd);
        if (!result.success) {
            return result;
        }
    }

    for (const auto& extra : target.extraCommands) {
        result = RunCommand(ToWide(extra));
        if (!result.success) {
            return result;
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
    std::wstring args;
    for (size_t i = 0; i < target.nvidiaSmiArgs.size(); ++i) {
        if (i > 0) {
            args += L" ";
        }
        args += ToWide(target.nvidiaSmiArgs[i]);
    }
    auto command = BuildCommand(L"nvidia-smi", args);
    auto result = RunCommand(command);
    if (result.success) {
        result.message = L"GPU target applied";
    }
    return result;
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

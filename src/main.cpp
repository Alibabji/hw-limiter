#include <cstdint>
#include <filesystem>
#include <string>
#include <system_error>

#ifdef _WIN32
#include <Windows.h>
#else
#include <iostream>
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#endif

#include "AppState.hpp"
#include "HardwareInfo.hpp"
#include "ProfileLoader.hpp"

#ifdef _WIN32

namespace {

constexpr wchar_t kWindowClassName[] = L"HardwareLimiterWindow";
constexpr int kCpuListId = 1001;
constexpr int kGpuListId = 1002;
constexpr int kApplyCpuButtonId = 1003;
constexpr int kApplyGpuButtonId = 1004;
constexpr int kRestoreButtonId = 1005;

struct UiElements {
    HWND cpuList = nullptr;
    HWND gpuList = nullptr;
    HWND statusLabel = nullptr;
};

AppState g_state;
UiElements g_ui;

std::filesystem::path GetExecutableDirectory() {
    wchar_t buffer[MAX_PATH];
    GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    std::filesystem::path path(buffer);
    return path.parent_path();
}

void UpdateStatus(const std::wstring& text) {
    if (g_ui.statusLabel) {
        SetWindowTextW(g_ui.statusLabel, text.c_str());
    }
}

void PopulateLists(HWND hwnd) {
    if (!g_ui.cpuList || !g_ui.gpuList) {
        return;
    }
    SendMessageW(g_ui.cpuList, LB_RESETCONTENT, 0, 0);
    for (size_t i = 0; i < g_state.cpuOptions.size(); ++i) {
        const auto& option = g_state.cpuOptions[i];
        std::wstring item(option.label.begin(), option.label.end());
        SendMessageW(g_ui.cpuList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    }
    SendMessageW(g_ui.gpuList, LB_RESETCONTENT, 0, 0);
    for (const auto& option : g_state.gpuOptions) {
        std::wstring item(option.label.begin(), option.label.end());
        SendMessageW(g_ui.gpuList, LB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item.c_str()));
    }

    std::wstring cpuText = L"Detected CPU: " + std::wstring(g_state.snapshot.cpu.name.begin(), g_state.snapshot.cpu.name.end());
    UpdateStatus(cpuText);
}

void InitializeApp(HWND hwnd) {
    if (g_state.initialized) {
        return;
    }
    HardwareInfoService infoService;
    g_state.snapshot = infoService.QueryHardware();

    ProfileLoader loader;
    std::filesystem::path profilePath = GetExecutableDirectory() / "profiles.json";
    try {
        g_state.profiles = loader.LoadFromFile(profilePath);
    } catch (const std::exception& ex) {
        std::string detail = ex.what();
        std::wstring message = L"Failed to load profiles: ";
        message += std::wstring(detail.begin(), detail.end());
        MessageBoxW(hwnd, message.c_str(), L"HardwareLimiter", MB_ICONERROR);
        return;
    }

    g_state.engine.Refresh(g_state.snapshot, g_state.profiles);
    g_state.cpuOptions = g_state.engine.CpuOptions();
    g_state.gpuOptions = g_state.engine.GpuOptions();
    g_state.initialized = true;
    PopulateLists(hwnd);
}

void HandleCpuSelection() {
    int index = static_cast<int>(SendMessageW(g_ui.cpuList, LB_GETCURSEL, 0, 0));
    if (index >= 0 && static_cast<size_t>(index) < g_state.cpuOptions.size()) {
        g_state.selectedCpu = g_state.cpuOptions[static_cast<size_t>(index)];
    }
}

void HandleGpuSelection() {
    int index = static_cast<int>(SendMessageW(g_ui.gpuList, LB_GETCURSEL, 0, 0));
    if (index >= 0 && static_cast<size_t>(index) < g_state.gpuOptions.size()) {
        g_state.selectedGpu = g_state.gpuOptions[static_cast<size_t>(index)];
    }
}

void ApplyCpuSelection() {
    if (!g_state.selectedCpu) {
        UpdateStatus(L"Select a CPU target first");
        return;
    }
    auto result = g_state.throttler.ApplyCpuTarget(*g_state.selectedCpu);
    UpdateStatus(result.message);
}

void ApplyGpuSelection() {
    if (!g_state.selectedGpu) {
        UpdateStatus(L"Select a GPU target first");
        return;
    }
    auto result = g_state.throttler.ApplyGpuTarget(*g_state.selectedGpu);
    UpdateStatus(result.message);
}

void RestoreDefaults() {
    auto result = g_state.throttler.RestoreDefaults();
    UpdateStatus(result.message);
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
        case WM_CREATE: {
            g_ui.cpuList = CreateWindowW(L"LISTBOX", nullptr,
                                         WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY,
                                         20, 40, 260, 200,
                                         hwnd, reinterpret_cast<HMENU>(kCpuListId), nullptr, nullptr);
            g_ui.gpuList = CreateWindowW(L"LISTBOX", nullptr,
                                         WS_CHILD | WS_VISIBLE | WS_BORDER | LBS_NOTIFY,
                                         320, 40, 260, 200,
                                         hwnd, reinterpret_cast<HMENU>(kGpuListId), nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Apply CPU", WS_CHILD | WS_VISIBLE,
                          20, 250, 120, 32, hwnd, reinterpret_cast<HMENU>(kApplyCpuButtonId), nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Apply GPU", WS_CHILD | WS_VISIBLE,
                          320, 250, 120, 32, hwnd, reinterpret_cast<HMENU>(kApplyGpuButtonId), nullptr, nullptr);
            CreateWindowW(L"BUTTON", L"Restore Defaults", WS_CHILD | WS_VISIBLE,
                          180, 300, 180, 32, hwnd, reinterpret_cast<HMENU>(kRestoreButtonId), nullptr, nullptr);
            g_ui.statusLabel = CreateWindowW(L"STATIC", L"Loading hardware info...",
                                             WS_CHILD | WS_VISIBLE,
                                             20, 10, 560, 24,
                                             hwnd, nullptr, nullptr, nullptr);
            InitializeApp(hwnd);
            return 0;
        }
        case WM_COMMAND: {
            switch (LOWORD(wParam)) {
                case kCpuListId:
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        HandleCpuSelection();
                    }
                    break;
                case kGpuListId:
                    if (HIWORD(wParam) == LBN_SELCHANGE) {
                        HandleGpuSelection();
                    }
                    break;
                case kApplyCpuButtonId:
                    ApplyCpuSelection();
                    break;
                case kApplyGpuButtonId:
                    ApplyGpuSelection();
                    break;
                case kRestoreButtonId:
                    RestoreDefaults();
                    break;
            }
            return 0;
        }
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                      _In_opt_ HINSTANCE /*hPrevInstance*/,
                      _In_ LPWSTR    /*lpCmdLine*/,
                      _In_ int       nCmdShow) {
    WNDCLASSW wc{};
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = kWindowClassName;
    wc.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    RegisterClassW(&wc);

    HWND hwnd = CreateWindowExW(
        0,
        kWindowClassName,
        L"Hardware Limiter",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 620, 380,
        nullptr,
        nullptr,
        hInstance,
        nullptr);

    if (!hwnd) {
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    return static_cast<int>(msg.wParam);
}

#else  // _WIN32

namespace {

std::filesystem::path GetExecutableDirectoryNonWin() {
#ifdef __APPLE__
    uint32_t size = 0;
    _NSGetExecutablePath(nullptr, &size);
    std::string buffer(size, '\0');
    if (_NSGetExecutablePath(buffer.data(), &size) == 0) {
        std::filesystem::path exePath(buffer.c_str());
        std::error_code ec;
        auto resolved = std::filesystem::weakly_canonical(exePath, ec);
        if (!ec) {
            return resolved.parent_path();
        }
        return exePath.parent_path();
    }
#endif
    return std::filesystem::current_path();
}

}  // namespace

int main() {
    std::cout << "Hardware Limiter - macOS preview\n";
    HardwareInfoService infoService;
    auto snapshot = infoService.QueryHardware();
    std::cout << "Detected CPU: " << snapshot.cpu.name << "\n";
    if (!snapshot.gpus.empty()) {
        std::string gpuName(snapshot.gpus.front().name.begin(), snapshot.gpus.front().name.end());
        std::cout << "Detected GPU: " << gpuName << "\n";
    } else {
        std::cout << "Detected GPU: (none reported)\n";
    }

    ProfileLoader loader;
    std::filesystem::path profilePath = GetExecutableDirectoryNonWin() / "profiles.json";
    if (!std::filesystem::exists(profilePath)) {
        profilePath = std::filesystem::current_path() / "resources" / "profiles.json";
    }
    ProfileDatabase db;
    try {
        db = loader.LoadFromFile(profilePath);
    } catch (const std::exception& ex) {
        std::cerr << "Failed to load profiles: " << ex.what() << "\n";
        return 1;
    }

    ProfileEngine engine;
    engine.Refresh(snapshot, db);
    const auto& cpuOptions = engine.CpuOptions();
    const auto& gpuOptions = engine.GpuOptions();
    std::cout << "Available downgrade targets (CPU): " << cpuOptions.size() << "\n";
    for (const auto& target : cpuOptions) {
        std::cout << " - " << target.label << "\n";
    }
    std::cout << "Available downgrade targets (GPU): " << gpuOptions.size() << "\n";
    for (const auto& target : gpuOptions) {
        std::cout << " - " << target.label << "\n";
    }

    std::cout << "Note: Applying throttling requires Windows at the moment.\n";
    return 0;
}

#endif  // _WIN32

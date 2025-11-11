#include "HardwareInfo.hpp"

#include <algorithm>
#include <cstring>
#include <memory>

#ifdef _WIN32
#include <Windows.h>
#include <dxgi1_6.h>
#include <intrin.h>
#include <wrl/client.h>
#elif defined(__APPLE__)
#include <array>
#include <cstdio>
#include <sstream>
#include <string>
#include <sys/sysctl.h>
#endif

namespace {

#ifdef _WIN32

using Microsoft::WRL::ComPtr;

unsigned long CountSetBits(ULONG_PTR bitMask) {
    unsigned long count = 0;
    while (bitMask) {
        bitMask &= (bitMask - 1);
        ++count;
    }
    return count;
}

CpuInfo ReadCpuInfo() {
    CpuInfo info;
    int cpuInfo[4] = {0};
    __cpuid(cpuInfo, 0);
    char vendor[13] = {0};
    *reinterpret_cast<int*>(vendor) = cpuInfo[1];
    *reinterpret_cast<int*>(vendor + 4) = cpuInfo[3];
    *reinterpret_cast<int*>(vendor + 8) = cpuInfo[2];
    info.vendor = vendor;

    int brand[4] = {0};
    char brandString[49] = {0};
    for (int i = 0; i < 3; ++i) {
        __cpuid(brand, 0x80000002 + i);
        std::memcpy(brandString + (i * 16), brand, sizeof(brand));
    }
    info.name = brandString;

    DWORD bufferSize = 0;
    GetLogicalProcessorInformationEx(RelationProcessorCore, nullptr, &bufferSize);
    std::unique_ptr<BYTE[]> buffer(new BYTE[bufferSize]);
    if (GetLogicalProcessorInformationEx(RelationProcessorCore,
                                         reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(buffer.get()),
                                         &bufferSize)) {
        BYTE* ptr = buffer.get();
        while (ptr < buffer.get() + bufferSize) {
            auto entry = reinterpret_cast<PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX>(ptr);
            if (entry->Relationship == RelationProcessorCore) {
                ++info.physicalCores;
                for (WORD group = 0; group < entry->Processor.GroupCount; ++group) {
                    info.logicalCores += CountSetBits(entry->Processor.GroupMask[group].Mask);
                }
            }
            ptr += entry->Size;
        }
    }
    return info;
}

std::wstring VendorFromId(UINT vendorId) {
    switch (vendorId) {
        case 0x10DE:
            return L"NVIDIA";
        case 0x1002:
        case 0x1022:
            return L"AMD";
        case 0x8086:
            return L"Intel";
        default: {
            wchar_t buffer[16];
            swprintf_s(buffer, L"0x%04X", vendorId);
            return buffer;
        }
    }
}

std::vector<GpuInfo> QueryGpus() {
    std::vector<GpuInfo> gpus;
    ComPtr<IDXGIFactory1> factory;
    if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&factory)))) {
        return gpus;
    }
    UINT index = 0;
    ComPtr<IDXGIAdapter1> adapter;
    while (factory->EnumAdapters1(index, &adapter) != DXGI_ERROR_NOT_FOUND) {
        DXGI_ADAPTER_DESC1 desc;
        if (SUCCEEDED(adapter->GetDesc1(&desc))) {
            GpuInfo gpu;
            gpu.name = desc.Description;
            gpu.vendor = VendorFromId(desc.VendorId);
            gpu.dedicatedVideoMemoryMB = static_cast<size_t>(desc.DedicatedVideoMemory / (1024 * 1024));
            gpu.adapterIndex = index;
            gpus.push_back(std::move(gpu));
        }
        adapter.Reset();
        ++index;
    }
    return gpus;
}

#elif defined(__APPLE__)

std::string Trim(const std::string& value) {
    const char* whitespace = " \t\r\n";
    auto start = value.find_first_not_of(whitespace);
    if (start == std::string::npos) {
        return {};
    }
    auto end = value.find_last_not_of(whitespace);
    return value.substr(start, end - start + 1);
}

std::wstring ToWide(const std::string& value) {
    return std::wstring(value.begin(), value.end());
}

CpuInfo ReadCpuInfoMac() {
    CpuInfo info;
    size_t length = 0;
    if (sysctlbyname("machdep.cpu.brand_string", nullptr, &length, nullptr, 0) == 0 && length > 0) {
        std::string brand(length, '\0');
        sysctlbyname("machdep.cpu.brand_string", brand.data(), &length, nullptr, 0);
        if (!brand.empty() && brand.back() == '\0') {
            brand.pop_back();
        }
        info.name = brand;
    }

    char vendorBuffer[64] = {0};
    length = sizeof(vendorBuffer);
    if (sysctlbyname("machdep.cpu.vendor", vendorBuffer, &length, nullptr, 0) == 0) {
        info.vendor = vendorBuffer;
    } else if (info.name.find("Apple") != std::string::npos) {
        info.vendor = "Apple";
    }

    uint32_t logical = 0;
    length = sizeof(logical);
    if (sysctlbyname("hw.logicalcpu", &logical, &length, nullptr, 0) == 0) {
        info.logicalCores = logical;
    }

    uint32_t physical = 0;
    length = sizeof(physical);
    if (sysctlbyname("hw.physicalcpu", &physical, &length, nullptr, 0) == 0) {
        info.physicalCores = physical;
    }
    return info;
}

std::vector<GpuInfo> QueryGpusMac() {
    std::vector<GpuInfo> gpus;
    FILE* pipe = popen("system_profiler SPDisplaysDataType", "r");
    if (!pipe) {
        return gpus;
    }
    std::array<char, 512> buffer{};
    GpuInfo current;
    while (fgets(buffer.data(), buffer.size(), pipe)) {
        std::string line = Trim(buffer.data());
        if (line.rfind("Chipset Model:", 0) == 0) {
            if (!current.name.empty()) {
                gpus.push_back(current);
                current = GpuInfo{};
            }
            std::string value = Trim(line.substr(std::strlen("Chipset Model:")));
            current.name = ToWide(value);
            if (current.vendor.empty()) {
                current.vendor = current.name;
            }
        } else if (line.rfind("Vendor:", 0) == 0) {
            std::string value = Trim(line.substr(std::strlen("Vendor:")));
            current.vendor = ToWide(value);
        } else if (line.rfind("VRAM", 0) == 0) {
            auto colon = line.find(':');
            if (colon != std::string::npos) {
                std::string value = Trim(line.substr(colon + 1));
                try {
                    size_t numberEnd = 0;
                    double amount = std::stod(value, &numberEnd);
                    std::string unit = Trim(value.substr(numberEnd));
                    if (unit.find("GB") != std::string::npos) {
                        amount *= 1024.0;
                    }
                    current.dedicatedVideoMemoryMB = static_cast<size_t>(amount);
                } catch (const std::exception&) {
                    // ignore parse errors
                }
            }
        }
    }
    if (!current.name.empty()) {
        gpus.push_back(current);
    }
    pclose(pipe);
    return gpus;
}

#endif  // platform selection

}  // namespace

HardwareSnapshot HardwareInfoService::QueryHardware() const {
    HardwareSnapshot snapshot;
#ifdef _WIN32
    snapshot.cpu = ReadCpuInfo();
    snapshot.gpus = QueryGpus();
#elif defined(__APPLE__)
    snapshot.cpu = ReadCpuInfoMac();
    snapshot.gpus = QueryGpusMac();
#else
    snapshot.cpu.name = "Unsupported platform";
#endif
    return snapshot;
}

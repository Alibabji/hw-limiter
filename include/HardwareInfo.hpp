#pragma once

#include <string>
#include <vector>

struct CpuInfo {
    std::string name;
    std::string vendor;
    unsigned logicalCores = 0;
    unsigned physicalCores = 0;
};

struct GpuInfo {
    std::wstring name;
    std::wstring vendor;
    size_t dedicatedVideoMemoryMB = 0;
    unsigned adapterIndex = 0;
};

struct HardwareSnapshot {
    CpuInfo cpu;
    std::vector<GpuInfo> gpus;
};

class HardwareInfoService {
public:
    HardwareSnapshot QueryHardware() const;
};

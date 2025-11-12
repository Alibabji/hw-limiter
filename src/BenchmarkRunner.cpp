#include "BenchmarkRunner.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstring>
#include <random>
#include <thread>
#include <vector>

#include "AppState.hpp"

#ifdef _WIN32
#include <Windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")
#endif

namespace {

BenchmarkResultData MakeResult(double score, const char* unit, std::string details) {
    BenchmarkResultData result;
    result.score = score;
    result.unit = unit ? unit : "";
    result.details = std::move(details);
    return result;
}

}  // namespace

BenchmarkReport BenchmarkRunner::Run(const HardwareSnapshot& snapshot) const {
    BenchmarkReport report;
    report.cpu = RunCpuBenchmark(snapshot);
    report.gpu = RunGpuBenchmark(snapshot);
    return report;
}

std::optional<BenchmarkResultData> BenchmarkRunner::RunCpuBenchmark(const HardwareSnapshot& snapshot) const {
    using namespace std::chrono;
    unsigned threadCount = snapshot.cpu.logicalCores;
    if (threadCount == 0) {
        threadCount = std::thread::hardware_concurrency();
    }
    if (threadCount == 0) {
        threadCount = 1;
    }
    const unsigned threads = threadCount;
    const size_t elements = 1 << 18;  // 262k elements
    const int iterations = 200;

    std::vector<double> a(elements);
    std::vector<double> b(elements);
    std::mt19937_64 rng(42);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    for (size_t i = 0; i < elements; ++i) {
        a[i] = dist(rng);
        b[i] = dist(rng);
    }

    std::atomic<uint64_t> operations = 0;
    auto worker = [&](unsigned /*index*/) {
        double acc = 0.0;
        for (int iter = 0; iter < iterations; ++iter) {
            for (size_t i = 0; i < elements; ++i) {
                acc += a[i] * b[i];
            }
        }
        operations += static_cast<uint64_t>(iterations) * elements * 2;
        return acc;
    };

    auto start = high_resolution_clock::now();
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (unsigned t = 0; t < threads; ++t) {
        workers.emplace_back(worker, t);
    }
    for (auto& th : workers) {
        th.join();
    }
    auto end = high_resolution_clock::now();
    const double seconds = duration<double>(end - start).count();
    if (seconds <= 0.0) {
        return std::nullopt;
    }
    const double gflops = (operations.load() / seconds) / 1e9;
    std::string details = "Threads: " + std::to_string(threads) +
                          ", ops: " + std::to_string(operations.load()) +
                          ", time: " + std::to_string(seconds) + "s";
    return MakeResult(gflops, "GFLOPS", details);
}

std::optional<BenchmarkResultData> BenchmarkRunner::RunGpuBenchmark(const HardwareSnapshot& snapshot) const {
#ifdef _WIN32
    using Microsoft::WRL::ComPtr;
    if (snapshot.gpus.empty()) {
        return std::nullopt;
    }

    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
#ifdef _DEBUG
    flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL level;
    HRESULT hr = D3D11CreateDevice(nullptr,
                                   D3D_DRIVER_TYPE_HARDWARE,
                                   nullptr,
                                   flags,
                                   nullptr,
                                   0,
                                   D3D11_SDK_VERSION,
                                   &device,
                                   &level,
                                   &context);
    if (FAILED(hr)) {
        return std::nullopt;
    }

    static const char* kShaderSrc = R"(
RWStructuredBuffer<float> BufferOut : register(u0);
[numthreads(256, 1, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
    float value = BufferOut[tid.x];
    [unroll(256)]
    for (uint i = 0; i < 4096; ++i) {
        value = mad(value, 1.000001f, 0.000001f);
    }
    BufferOut[tid.x] = value;
}
)";

    ComPtr<ID3DBlob> shaderBlob;
    ComPtr<ID3DBlob> errorBlob;
    hr = D3DCompile(kShaderSrc,
                    strlen(kShaderSrc),
                    nullptr,
                    nullptr,
                    nullptr,
                    "main",
                    "cs_5_0",
                    0,
                    0,
                    &shaderBlob,
                    &errorBlob);
    if (FAILED(hr)) {
        return std::nullopt;
    }

    ComPtr<ID3D11ComputeShader> computeShader;
    hr = device->CreateComputeShader(shaderBlob->GetBufferPointer(),
                                     shaderBlob->GetBufferSize(),
                                     nullptr,
                                     &computeShader);
    if (FAILED(hr)) {
        return std::nullopt;
    }

    const UINT elements = 256 * 1024;
    D3D11_BUFFER_DESC bufferDesc{};
    bufferDesc.BindFlags = D3D11_BIND_UNORDERED_ACCESS;
    bufferDesc.ByteWidth = sizeof(float) * elements;
    bufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
    bufferDesc.StructureByteStride = sizeof(float);

    ComPtr<ID3D11Buffer> buffer;
    hr = device->CreateBuffer(&bufferDesc, nullptr, &buffer);
    if (FAILED(hr)) {
        return std::nullopt;
    }

    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
    uavDesc.Buffer.NumElements = elements;
    ComPtr<ID3D11UnorderedAccessView> uav;
    hr = device->CreateUnorderedAccessView(buffer.Get(), &uavDesc, &uav);
    if (FAILED(hr)) {
        return std::nullopt;
    }

    context->CSSetShader(computeShader.Get(), nullptr, 0);
    ID3D11UnorderedAccessView* uavs[] = {uav.Get()};
    context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);

    ComPtr<ID3D11Query> disjoint;
    D3D11_QUERY_DESC disjointDesc{};
    disjointDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
    hr = device->CreateQuery(&disjointDesc, &disjoint);
    if (FAILED(hr)) {
        return std::nullopt;
    }

    D3D11_QUERY_DESC timestampDesc{};
    timestampDesc.Query = D3D11_QUERY_TIMESTAMP;
    ComPtr<ID3D11Query> startQuery;
    ComPtr<ID3D11Query> endQuery;
    device->CreateQuery(&timestampDesc, &startQuery);
    device->CreateQuery(&timestampDesc, &endQuery);

    const UINT dispatchCount = 2048;
    context->Begin(disjoint.Get());
    context->End(startQuery.Get());
    for (UINT i = 0; i < dispatchCount; ++i) {
        context->Dispatch(elements / 256, 1, 1);
    }
    context->End(endQuery.Get());
    context->End(disjoint.Get());
    context->Flush();

    D3D11_QUERY_DATA_TIMESTAMP_DISJOINT disjointData{};
    while (context->GetData(disjoint.Get(), &disjointData, sizeof(disjointData), 0) == S_FALSE) {
        Sleep(0);
    }
    UINT64 startTime = 0;
    while (context->GetData(startQuery.Get(), &startTime, sizeof(startTime), 0) == S_FALSE) {
        Sleep(0);
    }
    UINT64 endTime = 0;
    while (context->GetData(endQuery.Get(), &endTime, sizeof(endTime), 0) == S_FALSE) {
        Sleep(0);
    }
    if (disjointData.Disjoint || disjointData.Frequency == 0) {
        return std::nullopt;
    }
    const double gpuTimeSec = static_cast<double>(endTime - startTime) / static_cast<double>(disjointData.Frequency);
    if (gpuTimeSec <= 0.0) {
        return std::nullopt;
    }

    const double operations = static_cast<double>(dispatchCount) *
                              static_cast<double>(elements) *
                              4096.0 * 2.0;
    const double gflops = operations / gpuTimeSec / 1e9;
    std::string details = "Dispatches: " + std::to_string(dispatchCount) +
                          ", elements: " + std::to_string(elements) +
                          ", time: " + std::to_string(gpuTimeSec) + "s";

    ID3D11UnorderedAccessView* nullUAV[] = {nullptr};
    context->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
    context->CSSetShader(nullptr, nullptr, 0);

    return MakeResult(gflops, "GFLOPS", details);
#else
    (void)snapshot;
    return std::nullopt;
#endif
}

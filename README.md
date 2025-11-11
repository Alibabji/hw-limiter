# Hardware Limiter

Hardware Limiter is a cross-platform Qt application that inventories the local CPU/GPU, surfaces supported downgrade targets, and (on Windows) applies throttling so the machine can mimic slower hardware. It ships with a modern GUI, JSON-driven profile catalog, and command hooks that call `powercfg` plus `nvidia-smi` for best-effort enforcement. macOS builds share the same UI for planning and comparison, but enforcement remains Windows-only.

## Project Layout
- `src/MainWindow.cpp` - Qt Widgets-based UI shell that drives the hardware snapshot, profile engine, and throttling actions.
- `src/HardwareInfo.cpp` - Platform-specific hardware probes (CPUID/DXGI on Windows, `sysctl`/`system_profiler` on macOS).
- `src/Profile*` - JSON loader, matching engine, and throttling adapters.
- `include/` - Public headers and a minimal JSON parser (`SimpleJson.hpp`).
- `resources/profiles.json` - Downgrade catalog; copy this next to the executable/app bundle.
- `docs/ARCHITECTURE.md` / `docs/SUPPORTED_TARGETS.md` - High-level design plus the currently supported SKU->target matrix.

## Dependencies
- CMake 3.24+
- Qt 6.5+ (Widgets module)
- Windows: Visual Studio 2022 (Desktop C++), `nvidia-smi` on `PATH`, administrator rights to change power plans.
- macOS: Xcode Command Line Tools + `brew install qt`.

## Build Instructions
### Windows (MSVC + Qt)
1. Install Qt 6 (add the MSVC 2022 component). Note the Qt installation path (e.g., `C:\Qt\6.6.2\msvc2022_64`).
2. Open an **x64 Native Tools for VS 2022** prompt and set `CMAKE_PREFIX_PATH`:
   ```cmd
   set CMAKE_PREFIX_PATH=C:\Qt\6.6.2\msvc2022_64
   ```
3. Configure and build:
   ```cmd
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```
4. The output folder contains `HardwareLimiter.exe`, `profiles.json`, and the Qt runtime DLLs (use `windeployqt` if you need redistribution).

### macOS (Clang + Qt)
1. `brew install qt`
2. Configure and build:
   ```bash
   cmake -S . -B build -G "Unix Makefiles" -DCMAKE_PREFIX_PATH="$(brew --prefix qt)"
   cmake --build build
   ```
3. The build produces `HardwareLimiter.app`; copy `profiles.json` next to the bundle or run `macdeployqt build/HardwareLimiter.app` to prepare a standalone app.

## Running & Permissions
- Double-click `HardwareLimiter.exe` (right-click -> **Run as administrator**) or launch `HardwareLimiter.app`.
- The top banner lists detected hardware; CPU/GPU downgrade lists light up when the current machine matches entries in `resources/profiles.json`.
- On Windows, clicking **Apply** executes the associated `powercfg` / `nvidia-smi` commands. On macOS and unsupported GPUs the app will show a descriptive message instead of attempting throttling.
- Use **Restore Defaults** to revert the active power plan and clear clocks.

## Supported Hardware Profiles
See `docs/SUPPORTED_TARGETS.md` for the authoritative list. The current catalog includes:

### CPUs
| Host SKU | Downgrade Targets |
| --- | --- |
| Intel Core i7-13700K | i7-10700 (75% cap, boost 4.7 GHz), i7-6700 (60% cap, boost 4.0 GHz) |
| AMD Ryzen 9 7950X | Ryzen 7 5800X (70% cap, boost 4.5 GHz) |

### GPUs
| Host SKU | Downgrade Targets |
| --- | --- |
| NVIDIA RTX 4090 | RTX 3080 (1710 MHz / 320 W), RTX 3070 (1725 MHz / 240 W) |
| NVIDIA RTX 3080 | RTX 3060 Ti (1665 MHz / 200 W) |

## Customizing Profiles
- Extend `resources/profiles.json` with additional `cpuProfiles` / `gpuProfiles`. Each profile lists the substrings required to match a host (`matchTokens`) and the downgrade presets (`targets`).
- CPU targets can declare `maxFrequencyMHz`, `maxPercent`, logical/physical limits, and optional `extraCommands` (executed sequentially).
- GPU targets currently focus on NVIDIA devices via `nvidia-smi` arguments; add ADLX/Intel Arc commands if you need broader coverage.
- Keep the file ASCII-only (the lightweight parser ignores full Unicode) and ship an updated copy next to the binary.

## Limitations & Next Steps
- Enforcement is Windows-only; macOS/Linux builds are for planning, comparison, and profile editing.
- GPU throttling supports NVIDIA cards via `nvidia-smi`; AMD/Intel backends require vendor-specific tooling.
- Profiles are unsigned JSON; consider signing or hashing them before distributing the app.
- No persistent settings yet—consider storing the last-applied profile and adding telemetry around success/failure states.

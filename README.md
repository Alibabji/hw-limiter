# Hardware Limiter

Hardware Limiter is a Windows-only Qt application that inventories the local CPU/GPU, surfaces every supported downgrade option, and (with elevation) applies throttling so the machine can mimic slower hardware. It relies on Windows power plans for CPUs (`powercfg`) plus NVIDIA’s `nvidia-smi` for GPUs. When a requested cap is extreme, the GUI displays an explicit “use at your own risk” warning before executing any command.

## Project Layout
- `src/MainWindow.cpp` – Qt Widgets UI, safety prompts, and orchestration logic.
- `src/HardwareInfo.cpp` – Windows hardware probes (CPUID + DXGI).
- `src/Profile*` – JSON-driven catalog loader, matching engine, and throttling adapters.
- `include/` – Public headers plus a lightweight JSON helper.
- `resources/profiles.json` – Auto-generated downgrade catalog (see `scripts/generate_profiles.py`).
- `docs/ARCHITECTURE.md` / `docs/SUPPORTED_TARGETS.md` – Design overview and hardware coverage tables.

## Dependencies (Windows only)
- Visual Studio 2022 (Desktop C++ workload)
- CMake 3.24+
- Qt 6.5+ (Widgets, MSVC 64-bit kit)
- NVIDIA drivers with `nvidia-smi` accessible (for GPU throttling)

## Build & Package
1. Install Qt and note the kit path, e.g. `C:\Qt\6.6.2\msvc2022_64`.
2. In an **x64 Native Tools for VS 2022** prompt:
   ```cmd
   set CMAKE_PREFIX_PATH=C:\Qt\6.6.2\msvc2022_64
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```
3. Deploy dependencies beside the executable:
   ```cmd
   windeployqt --release build\\Release\\HardwareLimiter.exe
   ```
4. Distribute the resulting folder (contains `HardwareLimiter.exe`, Qt DLLs, and `profiles.json`). macOS/Linux builds are intentionally unsupported.

## Running & Permissions
- Launch `HardwareLimiter.exe` via **Run as administrator** so `powercfg`/`nvidia-smi` can change system limits.
- The top banner lists detected CPUs/GPUs and highlights which downgrade tiers are valid.
- Selecting an aggressive tier triggers a confirmation dialog reminding the user that all responsibility lies with them before any command executes.
- **Restore Defaults** immediately reapplies 100% CPU power and clears GPU clock/power overrides.

## Supported Hardware Families
(Full matrix in `docs/SUPPORTED_TARGETS.md`; generated via `scripts/generate_profiles.py`.)

- **Intel Core**: All Core i3/i5/i7/i9 processors from 6th through 14th generation (desktop & mobile naming). Each can mimic up to three prior generations of the same class.
- **AMD Ryzen**: Ryzen 3/5/7/9 families across the 1000, 2000, 3000, 4000, 5000, 7000, and 8000 series. Downgrade tiers step backward through earlier Zen generations.
- **NVIDIA GeForce GTX/RTX**: Every GTX 10/16 series card and every RTX 20/30/40 series card (including Ti/SUPER variants) released since 2016. Each GPU can be capped to several earlier SKUs with pre-tuned clock and power limits.

If you need to regenerate or extend the catalog, edit `scripts/generate_profiles.py` and run it to rewrite `resources/profiles.json`.

## Customization & Safety
- `resources/profiles.json` entries contain `requiresConfirmation` flags; add the flag to any new tier that could destabilize certain systems.
- CPU targets support `maxFrequencyMHz`, `maxPercent`, and optional `extraCommands` (executed in order, typically more `powercfg` tweaks).
- GPU targets declare `nvidiaSmiArgs`, which the app forwards to `nvidia-smi`.
- Only ASCII is supported inside the JSON file because of the minimal parser.

## Limitations & Next Steps
- GPU throttling is NVIDIA-only; AMD/Intel GPUs would require integrating ADLX or Arc Control CLIs.
- Profiles are unsigned JSON; consider signing or hashing before distributing binaries.
- Planned enhancements: persistence of the last applied tier, richer telemetry/diagnostics, and per-profile notes surfaced in the UI.

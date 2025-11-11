# Hardware Limiter

Hardware Limiter is a Windows-only C++ utility that inventories the local CPU/GPU, shows downgrade targets, and applies throttling so the machine can mimic slower hardware. It ships with a lightweight Win32 GUI, JSON-driven profile catalog, and command hooks that call `powercfg` and `nvidia-smi` for best-effort enforcement.

## Project Layout
- `src/` – Win32 entry point plus modules (`HardwareInfo`, `ProfileLoader`, `ProfileEngine`, `PowerThrottler`).
- `include/` – Public headers and a compact JSON parser (`SimpleJson.hpp`).
- `resources/profiles.json` – Reference downgrade profiles with CPU/GPU limits and vendor commands.
- `docs/ARCHITECTURE.md` – Module-level design notes and roadmap.

## Build Instructions (Windows)
1. Install Visual Studio 2022 (Desktop C++ workload) and CMake 3.24+.
2. Open a **x64 Native Tools** prompt and run:
   ```cmd
   cmake -S . -B build -G "Visual Studio 17 2022" -A x64
   cmake --build build --config Release
   ```
3. Copy `build/Release/HardwareLimiter.exe` and `resources/profiles.json` to your distribution folder (the build step already drops a copy next to the binary).

## Build Instructions (macOS preview)
The macOS target is a CLI preview that reports detected hardware and compatible downgrade profiles. It does not apply throttling yet.

```bash
cmake -S . -B build -G "Unix Makefiles"
cmake --build build
./build/HardwareLimiter
```

## Running & Permissions
- Launch `HardwareLimiter.exe` via **Run as administrator** so the app can modify power plans and call `nvidia-smi`.
- The GUI displays detected hardware, compatible downgrade targets, and buttons to apply CPU/GPU limits or restore defaults.
- CPU throttling adjusts the active power scheme (`PROCTHROTTLEMAX`, `PROCFREQMAX`, and boost settings). GPU throttling currently shells out to `nvidia-smi`, so NVIDIA drivers must be installed and `nvidia-smi` must be on `PATH`.
- On macOS the binary prints detected hardware and available downgrade profiles for reference; throttling is limited to Windows for now.

## Customizing Profiles
- Edit `resources/profiles.json` to add/adjust SKUs. Each profile carries `matchTokens` (case-insensitive substrings) and a `targets` array describing available downgrades.
- `extraCommands` let you run additional `powercfg` or vendor CLIs; keep them idempotent and safe for elevated execution.
- After editing the file, rebuild or manually copy it beside `HardwareLimiter.exe`.

## Limitations & Next Steps
- GPU throttling supports NVIDIA devices only; AMD/Intel backends would need vendor CLIs (e.g., ADLX, Arc Control).
- The JSON parser is minimal and does not handle Unicode beyond ASCII; keep profile files ASCII-only.
- No persistence layer yet—restarts revert to stock behavior until you apply a target again.
- Consider adding signature checks for profile bundles before distributing the tool in a private repository.

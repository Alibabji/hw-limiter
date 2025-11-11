# Hardware Limiter Architecture

## Goals
- Detect host CPU/GPU models using Windows native APIs.
- Present GUI with selectable "profile targets" that map to reduced performance levels.
- Apply throttling through built-in Windows power settings (CPU), vendor CLIs (GPU), and optional custom scripts.

## Components
- **App (Win32 GUI)**: Handles message loop, renders basic controls, and orchestrates hardware queries + profile application.
- **HardwareInfo** (`src/HardwareInfo.*`): Uses `__cpuid`/DXGI on Windows and `sysctl`/`system_profiler` on macOS to expose `CpuInfo` / `GpuInfo` structs.
- **ProfileLoader** (`src/ProfileLoader.*`): Reads `resources/profiles.json` (RapidJSON) into `CpuProfile`/`GpuProfile` objects, including supported downgrade targets.
- **PowerThrottler** (`src/PowerThrottler.*`): Applies CPU targets via PowerCfg (max processor state, affinity, boost mode) and calls vendor hooks (e.g., `nvidia-smi -lgc`) when available.
- **ProfileEngine** (`src/ProfileEngine.*`): Matches live hardware to compatible downgrade options and exposes them to the UI.

## Data Flow
1. On startup, `HardwareInfo` captures CPU/GPU inventory.
2. `ProfileLoader` parses JSON describing each supported SKU and its allowable downgrade targets (IDs + settings payloads).
3. `ProfileEngine` cross-checks runtime hardware against the dataset, building a list of downgrade choices with estimated perf deltas.
4. User selects a target; `PowerThrottler` executes the associated actions (power plan tweaks, clock caps, optional scripts).
5. UI reflects current state and prompts for elevation if needed.

## Security Considerations
- Throttler runs privileged commands; we wrap them and log failures.
- Profiles can embed shell snippets, so repository defaults restrict to vetted entries.
- Revert path provided via "Restore defaults" button that reapplies original power plan and clears vendor limits.

## Platform Notes
- **Windows**: Full GUI + throttling pipeline (power plan + `nvidia-smi`).
- **macOS**: CLI preview. `HardwareInfo` falls back to `sysctl` + `system_profiler` to report CPU/GPU metadata, but throttling calls return "not supported". The shared code path (profiles, engine) still runs so results stay consistent with Windows.

## Next Steps
- Implement cross-vendor throttling adapters.
- Add persistence for last selection and telemetry for success/failure.

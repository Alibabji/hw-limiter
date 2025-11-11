# Supported Targets

This repository ships with a conservative reference set of downgrade profiles. Extend `resources/profiles.json` to broaden coverage, but keep the matrix below up to date so contributors know which combinations are intentionally supported and tested.

## CPUs
- **Intel Core i7-13700K** -> mimic:
  - **Core i7-10700** - Caps boost to 4.7 GHz, limits the power plan to 75% max processor state, and keeps 8C/16T active.
  - **Core i7-6700** - Reduces boost to 4.0 GHz, 60% processor state, and limits surfaces to 4C/8T for legacy parity.
- **AMD Ryzen 9 7950X** -> mimic:
  - **Ryzen 7 5800X** - Restricts boost to ~4.5 GHz and sets the processor state to 70% to stay within Zen 3 envelopes.

## GPUs
- **NVIDIA GeForce RTX 4090** -> mimic:
  - **RTX 3080** - Uses `nvidia-smi -lgc 1400,1400 -pl 320` (1710 MHz cap, 320 W power limit).
  - **RTX 3070** - Uses `nvidia-smi -lgc 1200,1200 -pl 240` (1725 MHz cap, 240 W power limit).
- **NVIDIA GeForce RTX 3080** -> mimic:
  - **RTX 3060 Ti** - Uses `nvidia-smi -lgc 1100,1100 -pl 200` (1665 MHz cap, 200 W power limit).

> **Note:** GPU throttling currently relies on `nvidia-smi` and therefore requires NVIDIA drivers. Adding AMD/Intel options involves wiring in ADLX, ROCm, or Arc Control equivalents.

# Supported Targets

The shipping `resources/profiles.json` (generated via `scripts/generate_profiles.py`) covers **every mainstream Intel Core / AMD Ryzen CPU released since 2016** and **every NVIDIA GeForce GTX 10/16 and RTX 20/30/40 SKU**. Each supported host exposes up to three downgrade tiers that walk backwards through earlier generations while staying within vendor tooling limits.

## CPU Coverage
| Vendor | Host Generations | Downgrade Options |
| --- | --- | --- |
| Intel Core i3/i5/i7/i9 | 6th through 14th Gen | Mimic any of the previous three generations within the same Core class (e.g., a 14th Gen i7 can throttle to 13th, 12th, and 11th Gen levels). Frequencies and processor-state caps are pre-tuned per generation. |
| AMD Ryzen 3/5/7/9 | 1000, 2000, 3000, 4000, 5000, 7000, 8000 series | Step down through older Zen generations (up to three tiers). Each target adjusts boost ceilings and processor-state percentages accordingly. |

CPU presets warn the user whenever the requested cap drops below ~40% max processor state or skips three generations.

## GPU Coverage
| Family | Included SKUs | Downgrade Options |
| --- | --- | --- |
| GeForce GTX | GTX 1050 / 1050 Ti / 1060 (3 & 6 GB) / 1070 / 1070 Ti / 1080 / 1080 Ti / 1650 / 1650 Super / 1660 / 1660 Super / 1660 Ti | Each card can mimic any earlier GTX SKU via `nvidia-smi -lgc` (clock caps) and `-pl` (power limits) tuned to that model’s reference specs. |
| GeForce RTX | RTX 2060 / 2060 Super / 2070 / 2070 Super / 2080 / 2080 Super / 2080 Ti / 3050 / 3060 / 3060 Ti / 3070 / 3070 Ti / 3080 / 3080 Ti / 3090 / 3090 Ti / 4060 / 4060 Ti / 4070 / 4070 Ti / 4080 / 4090 | Tiers walk back through earlier RTX generations (up to three steps). Aggressive targets (e.g., 4090 -> 2070) trigger an explicit warning before issuing the `nvidia-smi` commands. |

> **Reminder:** Throttling requires NVIDIA drivers with `nvidia-smi` on `PATH`. Adding AMD or Intel GPU coverage will require wiring in their vendor CLIs.

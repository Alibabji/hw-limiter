import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESOURCE_PATH = ROOT / "resources" / "profiles.json"

intel_generations = [
    (6, "6th Gen (Skylake)", 4000),
    (7, "7th Gen (Kaby Lake)", 4100),
    (8, "8th Gen (Coffee Lake)", 4300),
    (9, "9th Gen (Coffee Lake Refresh)", 4500),
    (10, "10th Gen (Comet Lake)", 4700),
    (11, "11th Gen (Rocket Lake)", 4900),
    (12, "12th Gen (Alder Lake)", 5100),
    (13, "13th Gen (Raptor Lake)", 5300),
    (14, "14th Gen (Raptor Lake Refresh)", 5400),
]

intel_segments = [
    ("i3", "Core i3", 60),
    ("i5", "Core i5", 70),
    ("i7", "Core i7", 80),
    ("i9", "Core i9", 85),
]

amd_series = [
    (1000, "1st Gen (Zen)", 3800),
    (2000, "2nd Gen (Zen+)", 4000),
    (3000, "3rd Gen (Zen 2)", 4300),
    (4000, "4th Gen (Zen 2 Mobile)", 4400),
    (5000, "5th Gen (Zen 3)", 4700),
    (7000, "7th Gen (Zen 4)", 5200),
    (8000, "8th Gen (Zen 4 Refresh)", 5300),
]

amd_segments = [
    (3, "Ryzen 3", 55),
    (5, "Ryzen 5", 65),
    (7, "Ryzen 7", 75),
    (9, "Ryzen 9", 82),
]

# GeForce models ordered roughly by performance (mid/high SKUs since 2016)
gpu_models = [
    ("GTX 1050", 1493, 75),
    ("GTX 1050 Ti", 1620, 75),
    ("GTX 1060 3GB", 1700, 120),
    ("GTX 1060 6GB", 1771, 120),
    ("GTX 1650", 1860, 75),
    ("GTX 1650 Super", 1950, 100),
    ("GTX 1660", 1860, 120),
    ("GTX 1660 Super", 1935, 125),
    ("GTX 1660 Ti", 1900, 120),
    ("GTX 1070", 1886, 150),
    ("GTX 1070 Ti", 1900, 180),
    ("GTX 1080", 2000, 180),
    ("GTX 1080 Ti", 2100, 250),
    ("RTX 2060", 1680, 160),
    ("RTX 2060 Super", 1770, 175),
    ("RTX 2070", 1770, 185),
    ("RTX 2070 Super", 1815, 215),
    ("RTX 2080", 1800, 215),
    ("RTX 2080 Super", 1815, 250),
    ("RTX 2080 Ti", 1755, 260),
    ("RTX 3050", 1777, 130),
    ("RTX 3060", 1780, 170),
    ("RTX 3060 Ti", 1800, 200),
    ("RTX 3070", 1815, 220),
    ("RTX 3070 Ti", 1890, 290),
    ("RTX 3080", 1710, 320),
    ("RTX 3080 Ti", 1710, 350),
    ("RTX 3090", 1700, 350),
    ("RTX 3090 Ti", 1860, 450),
    ("RTX 4060", 2460, 160),
    ("RTX 4060 Ti", 2535, 200),
    ("RTX 4070", 2475, 200),
    ("RTX 4070 Ti", 2610, 285),
    ("RTX 4080", 2505, 320),
    ("RTX 4090", 2520, 450),
]

def intel_cpu_profiles():
    profiles = []
    for seg_code, seg_label, base_percent in intel_segments:
        for gen, gen_label, freq in intel_generations:
            profile_id = f"intel-{seg_code}-gen{gen}"
            tokens = [
                f"{seg_code}-{gen}",
                f"core {seg_code} {gen}",
                f"core {seg_code} {gen}th",
                f"{seg_code} {gen}th",
            ]
            targets = []
            for delta in (1, 2, 3):
                target_gen = gen - delta
                if target_gen < intel_generations[0][0]:
                    continue
                _, target_label, target_freq = next(item for item in intel_generations if item[0] == target_gen)
                percent = max(35, base_percent - delta * 8)
                target = {
                    "id": f"{profile_id}-to-gen{target_gen}",
                    "label": f"Mimic Intel {seg_label} {target_label}",
                    "maxFrequencyMHz": target_freq,
                    "maxCores": 0,
                    "maxThreads": 0,
                    "maxPercent": percent,
                    "extraCommands": [
                        f"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE {min(4, delta + 1)}"
                    ],
                    "requiresConfirmation": bool(delta >= 3 or percent <= 40),
                }
                targets.append(target)
            if targets:
                profiles.append({
                    "id": profile_id,
                    "label": f"Intel {seg_label} {gen_label}",
                    "matchTokens": tokens,
                    "targets": targets,
                    "nominalFrequencyMHz": freq,
                })
    return profiles

def amd_cpu_profiles():
    profiles = []
    for seg_number, seg_label, base_percent in amd_segments:
        for series, series_label, freq in amd_series:
            series_code = str(series)[:2]
            profile_id = f"amd-ryzen{seg_number}-{series}"
            tokens = [
                f"ryzen {seg_number} {series_code}",
                f"ryzen {seg_number}-{series_code}",
                f"ryzen {seg_number} {series}",
            ]
            targets = []
            for delta in (1, 2, 3):
                series_index = next((i for i, tup in enumerate(amd_series) if tup[0] == series), None)
                target_index = None if series_index is None else series_index - delta
                if target_index is None or target_index < 0:
                    continue
                target_series, target_label, target_freq = amd_series[target_index]
                percent = max(32, base_percent - delta * 9)
                targets.append({
                    "id": f"{profile_id}-to-{target_series}",
                    "label": f"Mimic {seg_label} {target_label}",
                    "maxFrequencyMHz": target_freq,
                    "maxCores": 0,
                    "maxThreads": 0,
                    "maxPercent": percent,
                    "extraCommands": ["powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE 2"],
                    "requiresConfirmation": bool(delta >= 3 or percent <= 38),
                })
            if targets:
                profiles.append({
                    "id": profile_id,
                    "label": f"AMD {seg_label} {series_label}",
                    "matchTokens": tokens,
                    "targets": targets,
                    "nominalFrequencyMHz": freq,
                })
    return profiles

def gpu_profiles():
    profiles = []
    for idx, (name, freq, power) in enumerate(gpu_models):
        lower_name = name.lower()
        tokens = []
        base = lower_name.strip()
        tokens.append(base)
        tokens.append(f"geforce {base}")
        if "ti" in base:
            tokens.append(base.replace("ti", " ti"))
        if "super" in base:
            tokens.append(base.replace("super", " super"))
        tokens = list(dict.fromkeys(tokens))
        profile_id = "nvidia-" + lower_name.replace(" ", "-")
        targets = []
        for delta in (1, 2, 3):
            target_idx = idx - delta
            if target_idx < 0:
                continue
            target_name, target_freq, target_power = gpu_models[target_idx]
            tokens_variant = target_name
            requires_confirmation = delta >= 3 or target_power <= 140
            targets.append({
                "id": f"{profile_id}-to-{target_name.lower().replace(' ', '-')}",
                "label": f"Mimic NVIDIA GeForce {target_name}",
                "maxFrequencyMHz": target_freq,
                "powerLimitWatts": target_power,
                "nvidiaSmiArgs": [
                    "-lgc",
                    f"{target_freq},{target_freq}",
                    "-pl",
                    str(target_power),
                ],
                "requiresConfirmation": requires_confirmation,
            })
        profiles.append({
            "id": profile_id,
            "label": f"NVIDIA GeForce {name}",
            "matchTokens": tokens,
            "targets": targets,
            "nominalFrequencyMHz": freq,
            "nominalPowerWatts": power,
        })
    return profiles


def main():
    data = {
        "cpuProfiles": intel_cpu_profiles() + amd_cpu_profiles(),
        "gpuProfiles": gpu_profiles(),
    }
    RESOURCE_PATH.write_text(json.dumps(data, indent=2))
    print(f"Wrote {len(data['cpuProfiles'])} CPU profiles and {len(data['gpuProfiles'])} GPU profiles to {RESOURCE_PATH}")


if __name__ == "__main__":
    main()

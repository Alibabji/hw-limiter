import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESOURCE_PATH = ROOT / "resources" / "profiles.json"

# Utility maps for SKU generation
INTEL_SUFFIX = {"i3": 100, "i5": 600, "i7": 700, "i9": 900}
INTEL_VARIANTS = {
    "i3": ["", "T", "F"],
    "i5": ["", "F", "KF"],
    "i7": ["", "K", "KF"],
    "i9": ["", "K", "KF"],
}
AMD_SUFFIX = {3: 200, 5: 600, 7: 700, 9: 900}
AMD_VARIANTS = {
    3: [""],
    5: ["", "X"],
    7: ["", "X", "XT"],
    9: ["", "X", "XT"],
}

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
    ("GTX 1050", 1493, 75, 1.0),
    ("GTX 1050 Ti", 1620, 75, 1.1),
    ("GTX 1650", 1860, 75, 1.3),
    ("GTX 1650 Super", 1950, 100, 1.35),
    ("GTX 1060 3GB", 1700, 120, 1.4),
    ("GTX 1060 6GB", 1771, 120, 1.5),
    ("GTX 1660", 1860, 120, 1.6),
    ("GTX 1660 Super", 1935, 125, 1.7),
    ("GTX 1660 Ti", 1900, 120, 1.8),
    ("GTX 1070", 1886, 150, 1.9),
    ("GTX 1070 Ti", 1900, 180, 2.05),
    ("GTX 1080", 2000, 180, 2.2),
    ("GTX 1080 Ti", 2100, 250, 2.4),
    ("RTX 2060", 1680, 160, 2.3),
    ("RTX 2060 Super", 1770, 175, 2.45),
    ("RTX 3050", 1777, 130, 2.5),
    ("RTX 2070", 1770, 185, 2.6),
    ("RTX 2070 Super", 1815, 215, 2.8),
    ("RTX 3060", 1780, 170, 2.9),
    ("RTX 2080", 1800, 215, 3.0),
    ("RTX 3060 Ti", 1800, 200, 3.1),
    ("RTX 2080 Super", 1815, 250, 3.2),
    ("RTX 3070", 1815, 220, 3.3),
    ("RTX 2080 Ti", 1755, 260, 3.4),
    ("RTX 3070 Ti", 1890, 290, 3.5),
    ("RTX 3080", 1710, 320, 3.8),
    ("RTX 3080 Ti", 1710, 350, 4.0),
    ("RTX 3090", 1700, 350, 4.2),
    ("RTX 3090 Ti", 1860, 450, 4.4),
    ("RTX 4060", 2460, 160, 3.2),
    ("RTX 4060 Ti", 2535, 200, 3.5),
    ("RTX 4070", 2475, 200, 3.7),
    ("RTX 4070 Ti", 2610, 285, 3.9),
    ("RTX 4080", 2505, 320, 4.3),
    ("RTX 4090", 2520, 450, 4.8),
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
            suffix = INTEL_SUFFIX.get(seg_code, 100)
            sku_number = gen * 1000 + suffix
            for variant in INTEL_VARIANTS.get(seg_code, [""]):
                sku_text = f"{sku_number}{variant}".lower()
                tokens.extend([
                    f"{seg_code}{sku_text}",
                    f"{seg_code}-{sku_text}",
                    f"{seg_code} {sku_text}",
                    sku_text,
                ])
            tokens = list(dict.fromkeys(tokens))

            targets = []
            for lower_gen, target_label, target_freq in intel_generations:
                if lower_gen >= gen:
                    continue
                delta = gen - lower_gen
                percent = max(25, base_percent - delta * 6)
                target_suffix = INTEL_SUFFIX.get(seg_code, 100)
                target_sku = lower_gen * 1000 + target_suffix
                display_model = f"{seg_code.upper()}-{target_sku}"
                target = {
                    "id": f"{profile_id}-to-gen{lower_gen}",
                    "label": f"Mimic Intel {seg_label} {display_model}",
                    "maxFrequencyMHz": target_freq,
                    "maxCores": 0,
                    "maxThreads": 0,
                    "maxPercent": percent,
                    "extraCommands": [
                        f"powercfg /setacvalueindex SCHEME_CURRENT SUB_PROCESSOR PERFBOOSTMODE {min(4, delta)}"
                    ],
                    "requiresConfirmation": bool(delta >= 4 or percent <= 35),
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
            series_str = str(series)
            profile_id = f"amd-ryzen{seg_number}-{series}"
            tokens = []
            for prefix_len in range(1, len(series_str) + 1):
                prefix = series_str[:prefix_len]
                tokens.append(f"ryzen {seg_number} {prefix}")
                tokens.append(f"ryzen {seg_number}-{prefix}")
            tokens.append(f"ryzen {seg_number} {series}")
            sku_base = AMD_SUFFIX.get(seg_number, 500)
            approx_sku = series + sku_base
            for variant in AMD_VARIANTS.get(seg_number, [""]):
                sku_token = f"{approx_sku}{variant}".lower()
                tokens.extend([
                    f"ryzen {seg_number} {sku_token}",
                    f"ryzen {seg_number}-{sku_token}",
                    sku_token,
                ])
            tokens = list(dict.fromkeys(tokens))
            targets = []
            series_index = next((i for i, tup in enumerate(amd_series) if tup[0] == series), None)
            if series_index is None:
                continue
            for target_index in range(series_index - 1, -1, -1):
                target_series, target_label, target_freq = amd_series[target_index]
                delta = series_index - target_index
                percent = max(28, base_percent - delta * 7)
                target_base = AMD_SUFFIX.get(seg_number, 500)
                target_sku = target_series + target_base
                targets.append({
                    "id": f"{profile_id}-to-{target_series}",
                    "label": f"Mimic {seg_label} {target_sku}",
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
    for idx, (name, freq, power, perf) in enumerate(gpu_models):
        lower_name = name.lower()
        tokens = []
        base = lower_name.strip()
        tokens.extend([
            base,
            base.replace(" ", ""),
            base.replace(" ", "-"),
            f"geforce {base}",
        ])
        parts = base.split()
        if parts:
            tokens.append(" ".join(parts[-2:]))
        if parts and parts[0] in {"rtx", "gtx"}:
            tokens.append(" ".join(parts[:2]))
        if "ti" in base:
            tokens.append(base.replace("ti", " ti"))
        if "super" in base:
            tokens.append(base.replace("super", " super"))
        tokens = list(dict.fromkeys(tokens))
        profile_id = "nvidia-" + base.replace(" ", "-")

        candidates = [
            (t_name, t_freq, t_power, t_perf)
            for (t_name, t_freq, t_power, t_perf) in gpu_models
            if t_perf < perf
        ]
        candidates.sort(key=lambda item: item[3], reverse=True)
        targets = []
        max_targets = 8
        for t_name, t_freq, t_power, t_perf in candidates[:max_targets]:
            requires_confirmation = (perf - t_perf) >= 1.5 or t_power <= 130
            targets.append({
                "id": f"{profile_id}-to-{t_name.lower().replace(' ', '-')}",
                "label": f"Mimic NVIDIA GeForce {t_name}",
                "maxFrequencyMHz": t_freq,
                "powerLimitWatts": t_power,
                "nvidiaSmiArgs": [
                    "-lgc",
                    f"{t_freq},{t_freq}",
                    "-pl",
                    str(t_power),
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

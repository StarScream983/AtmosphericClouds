# DEVLOG

## 2026-08-22 — FBM for Coverage/Type, Octaves/Lacunarity/Gain exposed

Coverage and Type were single-octave noise. Replaced with FBM (`FbmSimplexNoise3D`/`FbmValueNoise3D`/`FbmPerlinNoise3D` in `Noise.ush`), dispatched via `SampleFbmNoise3D`.

New per-section params (mirrored for Coverage and Type):

- `CloudsCoverageOctaves` / `CloudsTypeOctaves` — int, 1-8, default 8
- `CloudsCoverageLacunarity` / `CloudsTypeLacunarity` — frequency multiplier per octave, 1-4, default 2
- `CloudsCoverageGain` / `CloudsTypeGain` — amplitude multiplier per octave, 0.1-0.9, default 0.5

Why Lacunarity/Gain: with gain=0.5, octave _N_ contributes only `0.5^N` of the total — going 6→8 octaves was barely visible (~1.6% vs ~0.4% contribution). Exposing gain lets higher octaves actually matter.

Plumbed through the full chain: `OrbisCloudsComponent.h` (UPROPERTY sliders) → `BuildPlanetRenderData()` → `FOrbisCloudsPlanetRenderData` → `FOrbisCloudsPS::FParameters` → `OrbisCloudsViewExtension.cpp` → `OrbisClouds.usf` globals → `Noise.ush::CalculateWeatherMap`. ImGui controls added per-section in `OrbisCloudsImGui.cpp`.

Bug hit: `FbmSimplexNoise3D` was called from `CalculateWeatherMap` before it was defined further down the file — HLSL/USF requires functions defined above their call site, no forward declarations. Fixed by moving the FBM block above `CalculateWeatherMap`.

## 2026-08-22 — Domain warp for Coverage, with toggle

Coverage's FBM shape still looked too regular/smooth. Added a domain warp: displace the sample position through another noise field before evaluating the final FBM, following IQ's `f(p + h(p))` (https://iquilezles.org/articles/warp/), adapted to 3D and scaled down from his full double-nested example (5 fbm calls) to a single level (3 fbm calls building a float3 offset + 1 final call).

`WarpPosition3D(Position, NoiseType, Seed, Lacunarity, Gain, WarpOctaves, WarpStrength)` in `Noise.ush` — offset vector built from 3 decorrelated fbm samples (one per axis, seed/position offset per axis), each at a fixed low octave count. Applied only to Coverage; Type untouched.

New toggle: `bCloudsCoverageUseWarp` (bool, off by default). `CalculateWeatherMap` branches between the plain-FBM path and the warped path.

Same plumbing chain as above, extended with the bool (converted to `uint32` at the `OrbisCloudsViewExtension.cpp` pass-parameter assignment, matching the existing `bDebugSolid`/`bDepthOcclusion` pattern).

## 2026-08-22 — Warp Strength/Octaves exposed

First warp pass used hardcoded `ORBIS_WARP_STRENGTH = 4.0` and `ORBIS_WARP_OFFSET_OCTAVES = 3`. Result was too wiggly/marbled — the warped position is already frequency-scaled (~1 unit ≈ one noise cell), so displacing by up to 4.0 units scrambled the sample across several noise cells instead of gently distorting the macro shape.

Both became live params:

- `CloudsCoverageWarpStrength` — float, 0-8, default lowered to 1.0
- `CloudsCoverageWarpOctaves` — int, 1-8, default 3 (unchanged)

Added a "Coverage Warp" section title above the warp checkbox in ImGui, with both sliders below it.

---

## 2026-08-24 — Adaptive raymarch step count (zenith/horizon)

HZD/Nubis scale raymarch step count by view angle: 64 steps at zenith, up to 96-128 at the horizon (more shell distance to cover at grazing angles). Their version assumes the camera is always near the ground looking up — angle from world "up" is a reliable proxy for shell chord length.

We don't have that assumption (camera can be anywhere: in the shell, below it, in space above it), so "up" has to be computed per-camera, not assumed as a world axis:

```
UpVector = normalize(CameraPosition - PlanetCenter)   // outward radial at the camera
d = abs(dot(RayDirection, UpVector))                  // 1 = straight up or down, 0 = horizon
StepCount = ceil(lerp(128, 64, d))
```

`abs()` is required: straight up (dot=+1) and straight down (dot=-1) are both short chords through the shell and should both get 64 steps — only near-perpendicular rays (grazing the shell, dot≈0) need the full 128.

Range not settled:

- `64-128` (2015 HZD)
- `54-96` (2017 Nubis).

---

## TEXTURE AUTHORING COMPONENT

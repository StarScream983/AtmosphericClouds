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

---

## GHOST PLANET BEHIND CAMERA

a ghost planet appeared asymetric to canera position opposite to actual camera.

### The problem:

IqRaySphereIntersection returns two ray-parameter values (near hit, far hit) by solving a quadratic. Whenever a root came out negative (the intersection point is behind the camera), the code clamped it to 0 with max(t, 0.). That clamp was fine when only one root was negative (camera inside the sphere: near root behind you, far root ahead — clamping the near one to 0 is correct). But it silently did the wrong thing when both roots were negative — which happens whenever a ray points away from the sphere entirely, even though the camera is clearly outside it. In that case both roots got clamped to (0, 0), and IntersectAtmosphereShell's hit test (OuterRadiusHit.x >= 0 && OuterRadiusHit.y >= 0) read (0, 0) as a valid hit at distance zero — a false positive.

That's the "ghost disc": wherever your camera's screen rays happened to point away from the planet, the shader rendered something anyway instead of correctly reporting "no intersection," with a real black gap on either side where rays that were even further off just failed the test outright.

### The fix:

the product of the two roots always equals C = |camera-to-center|² - radius², which depends only on the camera's distance from the sphere center — never on ray direction. That means "camera outside vs. inside the sphere" is one fixed fact for the whole frame; what varies per-pixel is only whether a same-signed pair of roots is positive (ray toward the sphere, real hit) or negative (ray away from it, no hit at all). I added an explicit check: if the far root is negative, both roots are behind the camera, so return a genuine miss (-1, -1) instead of clamping to (0, 0).

This was a bug shared by the original formula too (it did the same unconditional clamp) — it just took the earlier fixes (depth-occlusion gate, precision rewrite) clearing away the other issues before this one became visible on its own.

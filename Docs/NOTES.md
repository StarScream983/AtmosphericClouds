**Optional, not required.**

Plain 3D FBM on `shellPos` is enough to start. **Domain warp** helps if coverage/type look too regular or grid-like once shape 3D noise is in the mix.

**Minimal iq-style (one warp, cheap):**

```c
shellPos = inner shell point
q = fbm3d(shellPos * warpScale + planetSeed)

coverage = saturate(bias + fbm3d(shellPos * covScale + q * warpStrength))
type     = saturate(bias + fbm3d(shellPos * typeScale + q * warpStrength + 100))
```

One warp vector `q` (or `fbm3d` → 3 offsets) shared by both — breaks symmetry without a second full weather system.

**When to add it:** after base density works; if blobs look too uniform. **When to skip:** first wiring pass.

**Don’t** stack multiple warp layers on coverage/type — save that budget for shape/detail 3D textures.

---

## are all base noises the same as the files i gave you?

They are the same **algorithms**, but not bit-for-bit identical outputs.

- **Perlin:** same cubic lattice, 12 fixed gradients, quintic fade and trilinear interpolation.
- **Simplex:** same skewed tetrahedral lattice, four corner contributions and `32.0` normalization.
- **Value:** same hashed lattice values, quintic fade and trilinear interpolation.

Differences:

- Your source uses a signed `int32` hash; ours uses `uint`, so right-shift behavior and resulting patterns differ.
- Your source accepts a `Seed`; ours currently hardcodes `1337u`.
- Your source maps 31 hash bits to `[-1,1]`; ours uses the lower 24 bits for reliable float conversion.
- Our interpolation is algebraically expanded in places instead of chained `lerp`, but the math is equivalent.

So the visual character should match, but values at a given coordinate will not match the CPU preview exactly. For exact CPU/GPU parity, we must port its signed hash, hash-to-float conversion, and seed parameters exactly.

---

## What is `OrbisCloudsCVars.h`

```c++
#pragma once

#include "HAL/IConsoleManager.h"

extern TAutoConsoleVariable<int32> CVarOrbisCloudsWeatherMapChannel;
```

It **declares** (doesn't define) one CVar so other `.cpp` files can use it without pulling in the view extension.

## Where it's used

| File                               | Role                                                                         |
| ---------------------------------- | ---------------------------------------------------------------------------- |
| **`OrbisCloudsViewExtension.cpp`** | **Defines** the CVar and passes it to the shader as `WeatherMapChannel`      |
| **`OrbisCloudsImGui.cpp`**         | **Reads/writes** it for the "Displayed Weather Map" combo (Coverage vs Type) |

The CVar itself:

```cpp
TAutoConsoleVariable<int32> CVarOrbisCloudsWeatherMapChannel(
	TEXT("r.OrbisClouds.WeatherMapChannel"),
	0,
	TEXT("0 = Cloud Coverage. 1 = Cloud Type."),
	ECVF_RenderThreadSafe);
```

`DebugSolid` and `DepthOcclusion` stay `static` in that file — only `WeatherMapChannel` was extracted because ImGui needs it too.

## Why it exists

Standard UE pattern:

- **One `.cpp`** owns the CVar definition (linker needs exactly one)
- **Header with `extern`** lets ImGui toggle `r.OrbisClouds.WeatherMapChannel` without duplicating the variable or including view-extension internals

So the ImGui panel and the render pass stay in sync when you switch between coverage (R) and type (G) in the weather map debug view.

---

## Adaptive Raymarch Steps and LOD smapling:

### Adaptive Steps:

- large cheap steps: 64 at zenith, 128 at horizon, samples iso volume from coverage and 128^3 volume textures.
- small steps: switch to smaller steps when large steps hit non-zero density, aka hits iso volume, sample from 32^3 volume textures.

### LOD:

- low LOD: 128^3 volume textures.
- high LOD: 32^3 volume textures:

---

**Vol 9–11 + the SD volume-noise article** is the right core. Vol 1–4 are mostly “how I got here”; you already have the shell, depth clip, and march skeleton in `CloudScapes.ush`.

## Is it “just textures,” or does Yvan’s code matter?

**Both.** Textures alone won’t look like his screenshots if you only multiply one 3D sample by coverage. His **density function** is the main reason shapes read as clouds.

Right now you have:

```205:228:D:\PROJECTs\RND\CLoudsSurrogate\Plugins\AtmosphericClouds\Shaders\Private\CloudScapes.ush
// Density at ray distance t: 0.8 at inner (bottom), 0.1 at outer (top). Zero outside the annulus.
float CloudScapesDensityAt(...)
{
	...
	return lerp(0.8, 0.1, HeightFromBottom) * saturate(CloudDensity);
}
```

That’s a vertical gradient — no lumps, no erosion, no lighting. Yvan’s look comes from stacking **several deliberate stages**.

### What textures give you (~40%)

- **Base 3D** (R = Perlin–Worley, G/B/A = Worley LODs) — billowy macro shape
- **Detail 3D** — edge erosion
- **Weather map** (SD, seamless) — where clouds exist, peak height, density
- Vol 10 adds **HeightLUT** + **CloudShapeNoise** (2D displace, not curl)

Bad or generic FBM will still look mushy even with his code.

### What code gives you (~60% for shape + almost all of “realistic lighting”)

**Vol 9 `GetDensity`** — the important pattern:

1. Weather → coverage field (`WMc`, SA, DA)
2. Sample base 3D → build shape (`SN`) with Worley FBM from G/B/A
3. Sample detail 3D → **erode only at edges** (`remap(SN, DNmod, 1, 0, 1)`)
4. Height gradient sculpts bottom-heavy, wispy tops

That “shape then erode” split is HZD’s idea; it’s not one `texture3D * coverage`.

**Vol 10** is his best **shape** refinement over Vol 9:

| Tier     | Source                                         | Effect                                 |
| -------- | ---------------------------------------------- | -------------------------------------- |
| **Low**  | HeightLUT + weather `smoothstep`               | Regional cloud mass, vertical envelope |
| **Mid**  | 3D noise vs height-dependent thresholds        | Lumps inside the envelope              |
| **High** | **Displaced** 3D sample (shape noise warps UV) | Breaks tiling, sharpens edges          |

The displaced high tier (`FBMUVDisplace` → second 3D read) is a real code trick — not something the texture file alone provides.

**Vol 11** — less about shape, more about **not looking like grey fog**:

- Energy-conserving `integScatt` (scatter + transmittance)
- Adaptive steps inside cloud
- Proper composite (rgb = scattered light, a = transmittance)

**Presentation stack** (also code, not textures):

- Half-res march + **4-frame offset merge** (Vol 10) — hides banding
- Step **jitter**
- Dual HG (“silver lining”)
- **Powder / depth probability** (Vol 9 `OutScatterAbient`) — brighter bases
- Dual Beer (`max(prim, scnd)`) — softer self-shadow
- Depth occlusion

## Does Yvan improve on HZD?

**Not really at the algorithm level.** He’s a solid **UE4 classroom implementation** of HZD-ish ideas, with a few swaps:

| Yvan                             | HZD / Nubis                          |
| -------------------------------- | ------------------------------------ |
| Weather G = high-freq coverage   | G = **cloud type**                   |
| HeightLUT + Low/Mid/High tiers   | Stratus/cumulus/cumulonimbus presets |
| 2D shape noise **displace**      | **Curl** 2D                          |
| 2 sun steps, 256–800 march steps | Cone samples, reprojection, LOD      |
| 4-frame half-res merge           | Full temporal reprojection           |

His clouds look good because he runs the **full stack** (density stages + lighting + half-res/temporal), with **tuned remap constants** and **hand-authored SD assets** — not because he invented a better noise.

## Practical priority for CloudScapes

1. **SD** → base + detail 3D + weather (and HeightLUT/shape 2D if you follow Vol 10)
2. **Vol 9 `GetDensity`** — biggest shape jump
3. **Vol 10** displacement + HeightLUT — extra realism on silhouettes and variety
4. **Vol 11** march/light/composite — grey debug → actual clouds
5. Half-res + jitter when performance allows

**Bottom line:** His realism is **correct multi-stage density + erosion + lighting + presentation**, with **good SD textures** feeding it. Textures alone ≈ lumpy fog; code alone with procedural noise ≈ mush. You need both; Vol 9–11 is where the code side lives.

If you want to implement next, Vol 9 `GetDensity` ported into `CloudScapes.ush` with texture slots on the component is the highest-value step.

**No. Yvan’s code is not an improvement over HZD.**

It’s a **simplified UE4 recreation** of ideas from the HZD / Nubis papers — closer to “how to get a similar look in a classroom project” than “better than Guerrilla.”

**HZD is the reference.** Yvan copies the broad pipeline (spherical shell, weather map, Perlin–Worley + Worley LODs, detail erosion, Beer + HG + powder-ish terms) but **cuts or replaces** a lot of what makes Nubis production-grade:

- No real **temporal reprojection** (he uses half-res + 4-frame merge instead)
- Fewer **sun samples** (often 2 vs HZD’s cone sampling)
- **Weather G** = high-freq coverage, not HZD’s **cloud-type** blend
- **Shape noise displace** instead of HZD’s **curl** distortion
- **HeightLUT / Low/Mid/High** instead of HZD’s stratus/cumulus/cumulonimbus presets
- Weaker **LOD / horizon / optimization** story (Nubis 2017 stuff largely absent)
- No HZD split of **high clouds as 2D scrollers** above the volumetric layer

**Why his clouds still look good:** solid **density staging** (shape → edge erosion), reasonable lighting stack, tuned remaps, and **good SD-authored textures** — not because his shaders beat HZD.

**For your project:** treat Yvan as a **practical UE porting guide** (especially Vol 9–11). Treat **HZD/Nubis papers** as the spec when you want correctness and the features Yvan left out.

---

YVAN code has a good SD authored textures, density sampling, tuned remaps, do you think we can do HZD oriented code then have an option swappable alternative as YVAN LEE method?

**Yes — that’s a sensible split**, and you don’t need two full renderers.

Treat **HZD as the default spec** (shell march, type/coverage, height presets, curl, lighting, reprojection when you add it). Treat **Yvan as an alternate density model** (and its asset set), not a second cloud system.

**What should be shared (one pipeline):**

- Spherical shell intersection + height fraction
- Ray march loop, jitter, depth clip
- Sun march + Beer / HG / transmittance composite
- View extension / pass structure

**What should be swappable (one fork):**

- **Density sampling** — HZD `GetDensity` vs Yvan Vol 9/10 `GetDensity`
- **Bound textures** — HZD: weather (R coverage, G type), base 3D, detail 3D, curl. Yvan: weather (dual coverage + peak/density), base 3D, detail 3D, HeightLUT, optional shape noise for displace
- **Tuning constants** — remap weights, erosion strength, global coverage/density

Base/detail 3D noises can often be **the same assets**; weather and the extra 2D maps differ.

**What’s awkward to hot-swap:**

- **Weather map semantics** — same PNG can’t mean “type” and “high-freq coverage” at once; use separate weather assets per mode or separate channels with clear authoring rules
- **Temporal** — HZD reprojection vs Yvan 4-frame half-res merge are different post-march paths; fine as quality presets later, not as a runtime “density mode” toggle

**Practical shape:**

- Enum / preset: `DensityModel = HZD | YvanLee`
- One `SampleCloudDensity(...)` that dispatches internally
- Component holds texture refs for both sets; only the active mode’s textures need to be set

**Verdict:** HZD-oriented core + Yvan as an optional density/authoring preset is a good design. You’re not cloning two engines — you’re offering **two ways to build density on the same march/light/composite stack**, which matches how the articles actually differ.

**YVAN targets the same look**, but it’s built differently from HZD, and he admits the default can look **too flat** until you tune it.

**HZD:** one explicit idea in the density stack — **lower density at the cloud base** so bottoms read thin and wispy, plus powder in lighting.

**Yvan (Vol 9):** soft bottoms come from **several knobs**, not one “wispy base” switch:

1. **Shape field `SA`** — `SRb` ramps up near the bottom of the layer (`remap(ph, 0, 0.07, …)`), `SRt` tightens the top → **big below, small above**.
2. **Density field `DA`** — `DRb` / `DRt` → **more mass low, less high** in the shell.
3. **Detail erosion** — `DNmod` blends inverted Worley with height (`lerp(DNfbm, 1-DNfbm, ph*1.8)`) → **breakup and wisp at edges/bases**.
4. **Lighting** — powder / depth + vertical probability (`OutScatterAbient`) → **brighter, softer bases** even when density isn’t zero.

He literally says if the **bottom looks too flat**, change parameters and it becomes “much more natural” (Vol 9).

**Vol 10** adds **HeightLUT** + Low/Mid/High tiers — another way to sculpt vertical envelope and edge breakup.

**Verdict:** Same **visual goal** as HZD soft bottoms; not the same formula. Yvan gets there with **vertical remaps + erosion + powder**, and it’s **tuning-dependent**. If you implement HZD mode, you’ll want the explicit bottom density falloff; in Yvan mode, you lean on `SA`/`DA`/detail inversion and powder instead.

---

    	const int StepCount = FindStepCount(RayDirection, PlanetCenterRelative);
    	const float CoarseStepSize = SegmentLength / StepCount;
    	const float ShellThickness = OuterRadius - InnerRadius;
    	float OpticalDepth = 0.0;
    	float WeightedHeight = 0.0;

    	// Adaptive raymarch: coarse steps (CoarseStepSize) until a step finds density, then step back one
    	// coarse step and switch to fine steps (0.3x) to re-approach and resolve that surface, reverting to
    	// coarse after 10 consecutive fine-step misses (walked out of the cloud). Same mechanism as Project-
    	// Marshmallow's compute-clouds.comp (github.com/mccannd/Project-Marshmallow) — verified against its
    	// source, and confirmed identical (same variable names, same 0.3x/10-miss constants) in an
    	// independent project, YueZhang1027/CIS5650-Final-Project-Frostnova's compute.comp, before
    	// implementing. We don't have Marshmallow's separate cheap/expensive density functions (cloudTest
    	// vs cloudHiRes) — CalculateWeatherMap is used for both the coarse check and the real accumulation,
    	// so the win here is skipping steps through empty sky and refining near the cloud surface, not a
    	// cheap/expensive split.
    	bool bNoHits = true;
    	int MissCount = 0;
    	int Iteration = 0;
    	const int MaxIterations = StepCount * 8; // safety cap: fine stepping (0.3x) can need ~3.3x more steps locally
    	float StepSize = CoarseStepSize;

    	for (float t = tRaymarchEnter; t < tRaymarchExit && Iteration < MaxIterations && ShellThickness > 0.0; t += StepSize, Iteration++)
    	{
    		const float Radius = SampleRadiusAlongRay(t, RayDirection, PlanetCenterRelative);
    		const float HeightFromBottom = (Radius - InnerRadius) / ShellThickness;
    		if (HeightFromBottom < 0.0 || HeightFromBottom > 1.0)
    		{
    			continue;
    		}

    		// Was: normalize(RayOrigin + RayDirection * t - PlanetCenterRelative) — same precision loss as
    		// the outer-shell case above (see ComputePreciseDirectionFromPlanetCenter in OrbisClouds.ush)
    		// whenever the camera is far from the planet.
    		const float3 DirectionFromPlanetCenterAtStep = ComputePreciseDirectionFromPlanetCenter(RayDirection, t, PlanetCenterRelativeDF);
    		const float3 SamplePosition = DirectionFromPlanetCenterAtStep * InnerRadius;

    		const float2 Density = CalculateWeatherMap(
    			SamplePosition,
    			InnerRadius,
    			CloudCoverageNoiseScale,
    			BaseNoiseType,
    			NoiseSeed,
    			NoiseOutputMin,
    			NoiseOutputMax,
    			CloudsCoverageOctaves,
    			CloudsCoverageLacunarity,
    			CloudsCoverageGain,
    			bCloudsCoverageUseWarp != 0u,
    			CloudsCoverageWarpStrength,
    			CloudsCoverageWarpOctaves,
    			CloudTypeNoiseScale,
    			CloudTypeNoiseType,
    			CloudTypeNoiseSeed,
    			CloudsTypeOctaves,
    			CloudsTypeLacunarity,
    			CloudsTypeGain);

    		if (Density.x > 0.0)
    		{
    			MissCount = 0;
    			if (bNoHits)
    			{
    				// First hit while coarse-stepping: step back one coarse step and switch to fine
    				// resolution. The for-loop's own increment (t += StepSize) runs right after this
    				// continue, using the just-shrunk StepSize — so the next iteration starts approaching
    				// this point again at fine resolution instead of accumulating the coarse-resolution
    				// sample.
    				t -= StepSize;
    				StepSize *= 0.3;
    				bNoHits = false;
    				continue;
    			}

    			const float StepContribution = StepSize * saturate(Density.x);
    			OpticalDepth += StepContribution;
    			WeightedHeight += StepContribution * HeightFromBottom;
    		}
    		else if (!bNoHits)
    		{
    			MissCount++;
    			if (MissCount >= 10)
    			{
    				// Walked out of the cloud during fine-stepping: revert to coarse resolution instead of
    				// continuing to fine-step through empty sky.
    				bNoHits = true;
    				StepSize /= 0.3;
    			}
    		}
    	}
    	// raymarch end -------------------------------------------------------------------------------

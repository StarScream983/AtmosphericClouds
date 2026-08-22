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

| File | Role |
|------|------|
| **`OrbisCloudsViewExtension.cpp`** | **Defines** the CVar and passes it to the shader as `WeatherMapChannel` |
| **`OrbisCloudsImGui.cpp`** | **Reads/writes** it for the "Displayed Weather Map" combo (Coverage vs Type) |

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
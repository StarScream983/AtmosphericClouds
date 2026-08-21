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
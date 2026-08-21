# OrbisClouds — Progression

## Overview

This plugin is a planetary atmospheric clouds plugin, following the Horizon Zero Dawn implementation, programmed through a custom render pass and the necessary optimization techniques, notably reprojection.

---

## Completed

### Module: `CloudScapesCore`

- Added **`CloudScapesCore`** runtime module (`PostConfigInit` loading phase).
- Registered shader directory mapping in `StartupModule()`:
  - Virtual path: `/Plugin/AtmosphericClouds`
  - Physical path: `Shaders/Private/`
- Added `Shaders/Private/Test.ush` for include/binding verification.

### Shader binding (tested)

- Confirmed plugin shader includes resolve via **Material Custom** node **Include File Paths** (`/Plugin/AtmosphericClouds/Test.ush`).
- `CloudScapesTestBinding()` callable from material graph after mapping is active.

### Project / plugin scaffold

- **`AtmosphericClouds`** renderer module (`Default` loading phase) — shell for view extension / RDG work.
- Parent project target files updated for **UE 5.8** (`BuildSettingsVersion.V7`).
- `Docs/` allowlisted in `.gitignore`.

### Module: `AtmosphericClouds` — data + render hook

#### `CloudScapesRenderTypes.h`

- **`struct FCloudScapesPlanetRenderData`** (`Source/AtmosphericClouds/Public/CloudScapesRenderTypes.h`)
  - `FVector PlanetCenter` — world position of planet center
  - `float PlanetRadius` — planet surface radius (cm)
  - `float AtmosphereInnerRadius` — inner shell radius (cm)
  - `float AtmosphereOuterRadius` — outer shell radius (cm)

#### `UCloudScapesComponent` (`CloudScapesComponent.h` / `.cpp`)

- **`class UCloudScapesComponent : public USceneComponent`**
- **`UCloudScapesComponent()`** — sets `PrimaryComponentTick.bCanEverTick = false`
- **`float PlanetRadius`** (default `500000.f`) — planet surface radius from component origin (cm)
- **`float CloudAltitudeBottom`** (default `50000.f`) — cloud layer bottom above surface (cm)
- **`float CloudAltitudeTop`** (default `150000.f`) — cloud layer top above surface (cm)
- **`FCloudScapesPlanetRenderData BuildPlanetRenderData() const`** — builds render data from component location and the three floats above; `AtmosphereInnerRadius = PlanetRadius + CloudAltitudeBottom`; `AtmosphereOuterRadius = PlanetRadius + max(CloudAltitudeTop, CloudAltitudeBottom + 1)`
- **`void OnRegister() override`** — calls `Super::OnRegister()`, then `UCloudScapesSubsystem::RegisterPlanet(this)`
- **`void OnUnregister() override`** — calls `UCloudScapesSubsystem::UnregisterPlanet(this)`, then `Super::OnUnregister()`

#### `UCloudScapesSubsystem` (`CloudScapesSubsystem.h` / `.cpp`)

- **`class UCloudScapesSubsystem : public UWorldSubsystem`**
- **`TWeakObjectPtr<UCloudScapesComponent> RegisteredPlanet`** (private) — single registered planet component
- **`void RegisterPlanet(UCloudScapesComponent* Component)`** — sets `RegisteredPlanet` if `Component` is valid
- **`void UnregisterPlanet(UCloudScapesComponent* Component)`** — clears `RegisteredPlanet` if it matches `Component`
- **`bool HasActivePlanet() const`** — returns true if `RegisteredPlanet` is still valid
- **`bool GetPlanetRenderData(FCloudScapesPlanetRenderData& OutPlanetData) const`** — fills `OutPlanetData` from the registered component; returns false if none

#### `FCloudScapesViewExtension` (`CloudScapesViewExtension.h` / `.cpp`)

- **`class FCloudScapesViewExtension : public FSceneViewExtensionBase`**
- **`FCloudScapesPlanetRenderData CachedPlanet`** (private) — single planet data copied each frame for the render pass
- **`bool bHasCachedPlanet`** (private) — true if planet data was gathered this frame
- **`FCloudScapesViewExtension(const FAutoRegister& AutoRegister)`** — calls `FSceneViewExtensionBase(AutoRegister)`
- **`void BeginRenderViewFamily(FSceneViewFamily& InViewFamily) override`** — game thread; clears `bHasCachedPlanet`, gets world from `InViewFamily.Scene`, calls `UCloudScapesSubsystem::GetPlanetRenderData(CachedPlanet)`
- **`void PrePostProcessPass_RenderThread(FRDGBuilder& GraphBuilder, const FSceneView& View, const FPostProcessingInputs& Inputs) override`** — render thread; returns if no cached planet; gets scene color; sets up `FCloudScapesPS` with `CachedPlanet`; draws fullscreen pass `"CloudScapes"` onto scene color

#### `AtmosphericClouds.cpp`

- **`TSharedPtr<FCloudScapesViewExtension> GCloudScapesViewExtension`** — keeps the extension alive
- **`FAtmosphericCloudsModule::StartupModule()`** — `GCloudScapesViewExtension = FSceneViewExtensions::NewExtension<FCloudScapesViewExtension>()`
- **`FAtmosphericCloudsModule::ShutdownModule()`** — `GCloudScapesViewExtension.Reset()`

#### `FCloudScapesPS` (`CloudScapesShader.h` / `.cpp`)

- **`class FCloudScapesPS : public FGlobalShader`**
- **`struct FParameters`** — `PlanetCenter`, `InnerRadius`, `OuterRadius`, `View`, render target bindings
- **`IMPLEMENT_GLOBAL_SHADER`** — registers pixel shader `MainPS` from `CloudScapes.usf`

#### Shaders (`Shaders/Private/`)

- **`CloudScapes.ush`** — `RaySphereIntersection`, `IntersectAtmosphereShell`, `ReconstructWorldRayDirection`
- **`CloudScapes.usf`** — fullscreen debug pass; ray vs shell annulus; cyan tint on hit, `discard` otherwise

#### Build (`AtmosphericClouds.Build.cs`)

- **`bTreatAsEngineModule = true`** — allows Renderer internal headers (e.g. `PostProcess/PostProcessInputs.h`)
- **`Renderer`** dependency — view extension / post-process inputs

#### Frame flow

1. Game thread: `BeginRenderViewFamily` → subsystem → one `FCloudScapesPlanetRenderData` into `CachedPlanet`
2. Render thread: scene renders → `PrePostProcessPass_RenderThread` → debug shell pass on scene color → post-process

#### Test in editor

1. Place one actor with **`UCloudScapesComponent`** at world origin
2. Move camera outside the shell — cyan atmospheric shell overlay where rays hit the annulus
3. GPU pass name: **`CloudScapes`**
- **`UCloudScapesComponent`** — blueprint-spawnable scene component with `PlanetRadius`, `CloudAltitudeBottom`, `CloudAltitudeTop` (cm). Registers with world subsystem on `OnRegister`.
- **`UCloudScapesSubsystem`** — `UWorldSubsystem` that gathers per-planet render data each frame.
- **`FCloudScapesPlanetRenderData`** — planet center + inner/outer atmosphere shell radii.
- **`FCloudScapesViewExtension`** — registered in `AtmosphericClouds` module `StartupModule()`. Gathers planets in `BeginRenderViewFamily`, draws debug shell in `PrePostProcessPass_RenderThread`.
- **`CloudScapes.usf` / `CloudScapes.ush`** — ray–sphere annulus intersection, cyan debug overlay (no noise/lighting yet).
- **`FCloudScapesPS`** global shader (`IMPLEMENT_GLOBAL_SHADER`).

### Test in editor

1. Place an actor with **`UCloudScapesComponent`** at world origin (defaults: 500 km planet radius, 50–150 km cloud layer).
2. Fly camera outside the shell — you should see a cyan atmospheric shell overlay where rays hit the annulus.
3. Render pass shows as **`CloudScapes`** in RenderDoc / GPU profiler.

---

## Next

- Depth-aware compositing (respect scene depth / occluders).
- Multi-planet iteration (currently renders first registered planet only).
- Per-world view extension lifecycle (optional: `FWorldSceneViewExtension` from subsystem).
- Noise density + HZD-style lighting march inside the shell.
- Temporal reprojection + disocclusion.

## Next

- [ ] `UCloudScapesComponent` + world subsystem (planet shell params)
- [ ] `FSceneViewExtension` + debug fullscreen / density pass
- [ ] HZD density model (noise, height presets, coverage)
- [ ] HZD lighting (Beer, HG, powder / in-scatter)
- [ ] Temporal reprojection + sparse low-res update

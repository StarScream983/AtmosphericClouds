## BASIC SHADER SETUP
1. base + shaders module + shaders binding
2. Ray sphere intersection through scene view extension
3. Depth occlusion and raymarched atmo test

## ORBIS CLOUDS IMPLEMENTATION
1. Shell gate     — ray hits cloud layer (annulus)
2. Density        — 3D noise (Perlin-Worley) in shell
3. View march     — integrate density along ray (fixed steps first)
4. Lighting       — light samples only where density > 0
5. Composite      — transmittance + luminance over scene
6. Optimizations  — sky-only pixels, adaptive steps, reprojection

## TO-DO:

+ cloud map: 
    - R: Coverage +- 0.5
    - G: Cloud type +- 1

+ Adaptive Raymarching Steps: 64 horizontal above head, 128 on the horizon
+ Reprojection
+ Quarter Resolution Buffer

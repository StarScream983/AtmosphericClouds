# Volumetric Overcast Clouds (Shadertoy Xttcz2)

**Author:** Valentino Galea (shaderbox)  
**Source:** https://www.shadertoy.com/view/Xttcz2  
**License:** Shadertoy / author terms — reference only.

> **Recovery note:** The original ~700-line paste from Shadertoy export was lost during the accidental folder wipe. Shadertoy blocks automated re-download (403 without API key). Re-export from the link above (Copy → paste shader) and replace this stub.

## What this shader is

Flat horizontal cloud slab (not planetary shell). Mashup of Magnus Wrenninge, HZD paper ideas, Scratchapixel sky, iq-style noise. Good visual reference; not Nubis production path.

## Key density snippet (from prior analysis)

```glsl
float density_func(vec3 pos, float h) {
    vec3 p = pos * .001 + cld_wind_dir;
    float dens = fbm_clouds(p * 2.032, 2.6434, .5, .5);
    dens *= smoothstep(cld_coverage, cld_coverage + .035, dens);
    // dens *= band(.2, .3, .5 + coverage_map * .5, h);  // commented — no texture map
    return dens;
}
```

`cld_coverage` is a constant (`0.3125`), not a weather texture.

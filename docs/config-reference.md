# Config Reference

All scene parameters are set in a JSON file passed to the renderer via `--config path/to/scene.json`.  Only fields you want to override need to be present — every field has a default value.

If `defaults.json` exists in the same directory as the scene file, it is loaded first and the scene file overlays it.  See [`scenes/defaults.json`](../scenes/defaults.json) for the project-wide defaults.

---

## Top-level structure

```jsonc
{
  "algorithm":  "mandelbrot",   // which algorithm to render
  "output":     "out.png",      // output path — extension controls format
  "bit_depth":  8,              // 8 or 16 (TIFF only); EXR is always float32
  "dpi":        300,
  "viewport":   { … },
  "threads":    0,              // 0 = hardware_concurrency
  "tile_size":  64,
  "aa":         1,              // antialiasing: per-axis supersampling factor (total spp = aa×aa); 1=off, 2=2×2 (4spp), 3=3×3 (9spp)
  // … algorithm-specific keys …
  "postprocess": { … }
}
```

> **Output naming:** at runtime the renderer replaces the filename portion of `output` with an auto-generated stem `[algo7]_YYYYMMDD_HHMM_NNNNN` (first 7 chars of algorithm name, timestamp, 5-digit counter).  The directory and extension from `output` are preserved.  A copy of the scene JSON is saved to `ImageSettings/[same stem].json`.

---

## Field reference

### Core

| Field | Type | Default | Description |
|---|---|---|---|
| `algorithm` | string | `"mandelbrot"` | Which algorithm to render — see [algorithms.md](algorithms.md). |
| `output` | string | `"output.png"` | Output path. Extension determines format: `.png`, `.tiff`/`.tif`, `.exr`. |
| `dpi` | int | `300` | DPI tag embedded in PNG/TIFF metadata (no effect on EXR). |
| `bit_depth` | int | `8` | `8` = 8-bit per channel PNG/TIFF; `16` = 16-bit. EXR is always 32-bit float. |

### Renderer

| Field | Type | Default | Description |
|---|---|---|---|
| `threads` | int | `0` | Worker thread count. `0` = auto-detect (hardware concurrency). |
| `tile_size` | int | `64` | Tile side length in pixels for the multithreaded tile renderer. |
| `aa` | int | `1` | Per-axis supersampling factor (total spp = `aa×aa`). `1` = off, `2` = 2×2 (4spp), `3` = 3×3 (9spp), `4` = 4×4 (16spp). |

### Viewport

Nested object mapping pixels to complex-plane coordinates.

```json
"viewport": {
  "width":    1920,
  "height":   1080,
  "real_min": -2.5,
  "real_max":  1.0,
  "imag_min": -1.25,
  "imag_max":  1.25
}
```

| Field | Type | Default | Description |
|---|---|---|---|
| `width` | int | `1920` | Output image width in pixels. |
| `height` | int | `1080` | Output image height in pixels. |
| `real_min` | float | `-2.5` | Left edge of the complex plane window. |
| `real_max` | float | `1.0` | Right edge. |
| `imag_min` | float | `-1.25` | Bottom edge. |
| `imag_max` | float | `1.25` | Top edge. |

### Escape-time (mandelbrot, julia, burning_ship, multibrot, tricorn)

| Field | Type | Default | Description |
|---|---|---|---|
| `max_iterations` | int | `1000` | Iteration cap before a point is considered in-set. **Deep zooms require a much higher value** — a viewport range of R units needs at least `max(2000, round(500/sqrt(R)))` iterations to avoid a blank image. |
| `escape_radius` | float | `2.0` | Magnitude threshold that marks a point as escaped. |
| `smooth_coloring` | bool | `true` | Enable Linas Vepstas continuous colouring (no banding). |
| `coloring_mode` | string | `"smooth"` | `"smooth"`, `"histogram_eq"`, or `"orbit_trap"`. |
| `color_cycle` | float | `64.0` | Palette repetitions across the iteration range. Higher = tighter bands. |

### Julia seed

| Field | Type | Default | Description |
|---|---|---|---|
| `julia_cr` | float | `-0.7` | Real part of the fixed seed c. |
| `julia_ci` | float | `0.27015` | Imaginary part of the fixed seed c. |

### Multibrot

| Field | Type | Default | Description |
|---|---|---|---|
| `multibrot_power` | float | `3.0` | Exponent n in zⁿ + c (any real value; 2.0 = Mandelbrot). |

### Newton

| Field | Type | Default | Description |
|---|---|---|---|
| `newton_power` | int | `3` | Polynomial degree n for f(z) = zⁿ − 1.  Must be ≥ 2. |
| `newton_tolerance` | float | `1e-6` | Convergence tolerance — root found when \|f(z)\| < tolerance. |
| `newton_saturation` | float | `0.8` | Saturation of basin hues (0 = grey, 1 = vivid). |

### Nova

| Field | Type | Default | Description |
|---|---|---|---|
| `nova_type` | string | `"mandelbrot"` | `"mandelbrot"` (z₀=1, c=pixel) or `"julia"` (z₀=pixel, c=seed). |
| `nova_power` | int | `3` | Polynomial degree. Must be ≥ 2; values below 2 produce a blank image. |
| `nova_relaxation` | float | `1.0` | Step-size multiplier R; ≠ 1 creates extra boundary detail. |
| `nova_seed_r` | float | `1.0` | Fixed c real part (julia mode only). Keep `sqrt(nova_seed_r²+nova_seed_i²)` ∈ [0.8, 2.5] to avoid a flat single-colour image. |
| `nova_seed_i` | float | `0.0` | Fixed c imaginary part (julia mode only). |
| `nova_max_iterations` | int | `256` | Per-pixel iteration cap. Increase to 512+ for `nova_type = "julia"` — the default 256 often produces a near-black image in Julia mode. |
| `nova_tolerance` | float | `1e-6` | Convergence tolerance for Newton step. |
| `nova_escape_radius` | float | `1e6` | Escape threshold magnitude. |
| `nova_saturation` | float | `0.8` | HSV saturation for converged basin colouring. |

### Simplex Noise FBM

| Field | Type | Default | Description |
|---|---|---|---|
| `noise_octaves` | int | `6` | Number of frequency layers summed. |
| `noise_persistence` | float | `0.5` | Amplitude falloff per octave (0–1). |
| `noise_lacunarity` | float | `2.0` | Frequency multiplier per octave. |
| `noise_scale` | float | `1.0` | Global frequency scale. |
| `noise_seed` | uint | `42` | RNG seed for the simplex permutation table. |

### Reaction-Diffusion (Gray-Scott)

| Field | Type | Default | Description |
|---|---|---|---|
| `rd_preset` | string | — | Named preset: `"coral"`, `"mitosis"`, `"worms"`, `"maze"`, `"spots"`, `"fingerprint"`. |
| `rd_feed` | float | preset | Feed rate F — overrides the preset value. |
| `rd_kill` | float | preset | Kill rate k — overrides the preset value. |
| `rd_steps` | int | `8000` | Number of Gray-Scott time steps to simulate. |
| `rd_seed` | uint | `42` | RNG seed for the initial random perturbation. |

### Plasma (Diamond-Square)

| Field | Type | Default | Description |
|---|---|---|---|
| `plasma_roughness` | float | `0.5` | Amplitude scale per level [0, 1]; lower = smoother. |
| `plasma_octaves` | int | `8` | Grid resolution = 2^octaves + 1; 8 → 257×257 grid. |
| `plasma_seed` | uint | `42` | RNG seed. |

### Strange Attractor (Clifford / De Jong)

| Field | Type | Default | Description |
|---|---|---|---|
| `attractor_type` | string | `"clifford"` | `"clifford"` or `"dejong"`. |
| `attractor_a` – `attractor_d` | float | see SceneConfig.h | Four shape parameters. For Clifford, keep all four in **[−2.0, 2.0]** with at least three having `\|value\| > 1.0` — combinations where `\|b\| < 1.0` and `\|d\| > 1.5` can produce a degenerate orbit and a blank image. |
| `attractor_iterations` | int | `8000000` | Orbit length (more = denser, smoother). |

### Ikeda Map

| Field | Type | Default | Description |
|---|---|---|---|
| `attractor_u` | float | `0.9` | Nonlinearity parameter. Best range: [0.75, 0.90]. |
| `attractor_iterations` | int | `8000000` | Orbit length. |

### Voronoi

| Field | Type | Default | Description |
|---|---|---|---|
| `voronoi_seeds` | int | `32` | Number of seed points. |
| `voronoi_seed` | uint | `42` | RNG seed for seed placement. |
| `voronoi_mode` | string | `"cells"` | `"cells"`, `"distance"`, or `"edge"`. |
| `voronoi_metric` | string | `"euclidean"` | `"euclidean"`, `"manhattan"`, or `"chebyshev"`. |

### L-System

| Field | Type | Default | Description |
|---|---|---|---|
| `ls_preset` | string | — | Named preset: `"plant"`, `"dragon"`, `"sierpinski"`, `"hilbert"`, `"tree"`. |
| `ls_axiom` | string | `"F"` | Initial axiom string (overrides preset). |
| `ls_rules` | string | — | Semicolon-separated production rules, e.g. `"F=FF;X=F+[[X]-X]"`. |
| `ls_iterations` | int | `5` | Number of rule-expansion steps. |
| `ls_angle` | float | `25.7` | Turtle turn angle in degrees. |
| `ls_fg_color` | string | `"#33CC55"` | Foreground (line) colour as `#RRGGBB`. |
| `ls_bg_color` | string | `"#080D08"` | Background colour as `#RRGGBB`. |

### Lyapunov Fractal

| Field | Type | Default | Description |
|---|---|---|---|
| `lyapunov_sequence` | string | `"AB"` | Sequence of `'A'` (real coord) and `'B'` (imag coord). |
| `lyapunov_warmup` | int | `200` | Logistic-map steps discarded before λ accumulation. |
| `lyapunov_iterations` | int | `1000` | Steps used to compute λ. |
| `lyapunov_seed_x` | float | `0.5` | Initial x₀ for the logistic map. |

Keep viewport within (2, 4) × (2, 4) — outside this range the logistic map diverges.

### Cyclic Cellular Automaton

| Field | Type | Default | Description |
|---|---|---|---|
| `cca_states` | int | `12` | Number of distinct states N (2–255). |
| `cca_neighborhood` | string | `"moore"` | `"moore"` (8-neighbor) or `"vonneumann"` (4-neighbor). |
| `cca_steps` | int | `500` | Simulation steps before rendering. |
| `cca_seed` | uint | `42` | RNG seed for the initial random grid. |

### Physarum (Slime Mould)

| Field | Type | Default | Description |
|---|---|---|---|
| `physarum_num_agents` | int | `200000` | Agent count. More → denser coverage. |
| `physarum_steps` | int | `400` | Simulation steps. More → more evolved networks. |
| `physarum_sensor_angle` | float | `45.0` | Forward sensor spread angle (degrees). Wider → branchy; narrower → highways. |
| `physarum_sensor_dist` | float | `9.0` | Sensor look-ahead distance in pixels. |
| `physarum_rotation_angle` | float | `45.0` | Agent turn amount per step (degrees). |
| `physarum_step_size` | float | `1.0` | Agent step size in pixels. |
| `physarum_deposit` | float | `5.0` | Trail chemical deposited per step. |
| `physarum_decay` | float | `0.95` | Trail evaporation factor per step. |
| `physarum_seed` | uint | `42` | RNG seed. |

### Palette

| Field | Type | Default | Description |
|---|---|---|---|
| `palette` | string | `"classic_mandelbrot"` | Named palette: `"fire"`, `"ice"`, `"electric"`, `"grayscale"`. |
| `palette_lab` | bool | `false` | Interpolate in CIELAB space (perceptually uniform; avoids grey mid-tones). |
| `palette_phase` | float | `0.0` | Cyclic hue rotation offset [0, 1). |
| `palette_type` | string | `"named"` | `"complementary"`, `"triadic"`, `"analogous"`, `"split_complementary"`, or `"custom"`. |
| `palette_hsl` | array | `[200, 0.8, 0.5]` | Base HSL for generated palette types: `[hue_deg, saturation, lightness]`. |
| `palette_stops` | array | — | Custom gradient stops. Requires `palette_type: "custom"`. Each entry: `{"pos": float, "color": "#RRGGBB"}`. |
| `aco_palette` | string | — | Path to an Adobe `.aco` swatch file. Overrides all other `palette_*` fields when set. |

```jsonc
// Named palette
{ "palette": "fire" }

// Generated palette
{ "palette_type": "triadic", "palette_hsl": [200, 0.85, 0.5] }

// Custom gradient
{
  "palette_type": "custom",
  "palette_stops": [
    { "pos": 0.0, "color": "#000000" },
    { "pos": 0.5, "color": "#FF6600" },
    { "pos": 1.0, "color": "#FFFFFF" }
  ]
}

// Adobe swatch import
{ "aco_palette": "palettes/my_swatches.aco" }
```

### Post-processing

Applied after the algorithm render, before saving.

```json
"postprocess": {
  "gamma":      1.0,
  "brightness": 0.0,
  "contrast":   1.0,
  "saturation": 1.0,
  "vignette":   0.0
}
```

| Field | Type | Default | Description |
|---|---|---|---|
| `gamma` | float | `1.0` | Gamma correction exponent. `1.0` = linear; `2.2` = standard display gamma. |
| `brightness` | float | `0.0` | Additive brightness offset (−1 to +1). |
| `contrast` | float | `1.0` | Contrast multiplier around mid-grey. |
| `saturation` | float | `1.0` | Saturation scale. `0` = greyscale, `>1` = boosted. |
| `vignette` | float | `0.0` | Radial corner darkening [0, 1]. |

---

## Minimal example

```json
{
  "algorithm": "mandelbrot",
  "viewport": { "width": 800, "height": 600 }
}
```

## Full example (Burning Ship crop)

```json
{
  "algorithm": "burning_ship",
  "viewport": {
    "width": 3840, "height": 2160,
    "real_min": -1.8, "real_max": -1.7,
    "imag_min": -0.09, "imag_max": 0.01
  },
  "max_iterations": 2000,
  "coloring_mode": "histogram_eq",
  "color_cycle": 32.0,
  "palette_type": "custom",
  "palette_lab": true,
  "palette_stops": [
    { "pos": 0.0, "color": "#0a0010" },
    { "pos": 0.4, "color": "#8b0000" },
    { "pos": 0.7, "color": "#ff6600" },
    { "pos": 1.0, "color": "#ffe0a0" }
  ],
  "postprocess": { "gamma": 2.0, "contrast": 1.15, "vignette": 0.3 },
  "aa": 4
}
```

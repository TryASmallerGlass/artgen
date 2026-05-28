# Config Reference

All scene parameters are set in a JSON file passed to the renderer via `--config path/to/scene.json`.  Only the fields you want to override need to be present; every field has a default value.

---

## Top-level structure

```json
{
  "algorithm":        "mandelbrot",
  "width":            1920,
  "height":           1080,
  "max_iterations":   500,
  "escape_radius":    2.0,
  "smooth_coloring":  true,
  "coloring_mode":    "smooth",
  "color_cycle":      64.0,
  "viewport":         { … },
  "palette":          { … },
  "aco_palette":      "path/to/swatches.aco",
  "postprocess":      { … },
  "output":           "output/image.png",
  "output_dpi":       150,
  "bit_depth":        8,
  "threads":          0,
  "tile_size":        64,
  "aa_samples":       1
}
```

---

## Field reference

### Core

| Field | Type | Default | Description |
|---|---|---|---|
| `algorithm` | string | `"mandelbrot"` | Which algorithm to render.  See [algorithms.md](algorithms.md) for all keys. |
| `width` | int | `1920` | Output image width in pixels. |
| `height` | int | `1080` | Output image height in pixels. |
| `output` | string | `"output/render.png"` | Output file path.  Extension determines format: `.png`, `.tiff`/`.tif`, `.exr`. |
| `output_dpi` | int | `96` | DPI tag embedded in PNG/TIFF metadata (no effect on EXR). |
| `bit_depth` | int | `8` | `8` = 8-bit per channel PNG/TIFF, `16` = 16-bit per channel.  EXR is always 32-bit float (fp16 optional). |

### Rendering quality

| Field | Type | Default | Description |
|---|---|---|---|
| `threads` | int | `0` | Worker thread count.  `0` = auto-detect (hardware concurrency). |
| `tile_size` | int | `64` | Tile side length in pixels for the multi-threaded tile renderer. |
| `aa_samples` | int | `1` | Anti-aliasing samples per pixel (grid super-sampling).  `1` = off, `4` = 2×2, `9` = 3×3, `16` = 4×4. |

### Escape-time parameters (Mandelbrot, Julia, Burning Ship)

| Field | Type | Default | Description |
|---|---|---|---|
| `max_iterations` | int | `500` | Maximum iteration count before a point is considered in-set. |
| `escape_radius` | float | `2.0` | Magnitude threshold that marks a point as escaped. |
| `smooth_coloring` | bool | `true` | Alias for `coloring_mode: "smooth"`.  Ignored when `coloring_mode` is set explicitly. |
| `coloring_mode` | string | `"smooth"` | `"smooth"`, `"histogram_eq"`, or `"orbit_trap"`. |
| `color_cycle` | float | `64.0` | Number of palette repetitions across the iteration range.  Higher = tighter colour bands. |

### Julia parameters

| Field | Type | Default | Description |
|---|---|---|---|
| `julia_cr` | float | `-0.7` | Real part of the fixed seed _c_. |
| `julia_ci` | float | `0.27015` | Imaginary part of the fixed seed _c_. |

### Newton parameters

| Field | Type | Default | Description |
|---|---|---|---|
| `newton_power` | int | `3` | Polynomial degree _n_ for _f(z) = zⁿ − 1_.  Must be ≥ 2. |
| `newton_tolerance` | float | `1e-6` | Convergence tolerance (root considered found when `|f(z)| < tolerance`). |
| `newton_saturation` | float | `0.8` | Colour saturation of basin hues (0 = grey, 1 = vivid). |

### Noise / FBM parameters

| Field | Type | Default | Description |
|---|---|---|---|
| `noise_octaves` | int | `6` | Number of frequency layers summed. |
| `noise_persistence` | float | `0.5` | Amplitude falloff factor per octave (0–1). |
| `noise_lacunarity` | float | `2.0` | Frequency multiplier per octave. |
| `noise_scale` | float | `1.0` | Global frequency scale (higher = more pattern repetitions per image). |
| `noise_seed` | uint | `0` | RNG seed for the simplex permutation table. |

### Reaction-diffusion parameters

| Field | Type | Default | Description |
|---|---|---|---|
| `rd_preset` | string | `"coral"` | Named preset: `"coral"`, `"mitosis"`, `"worms"`, `"maze"`, `"spots"`, `"fingerprint"`. |
| `rd_feed` | float | preset | Feed rate _F_ — overrides the preset value. |
| `rd_kill` | float | preset | Kill rate _k_ — overrides the preset value. |
| `rd_steps` | int | `8000` | Number of Gray-Scott time steps to simulate. |
| `rd_seed` | uint | `0` | RNG seed for the initial random perturbation. |

### L-System parameters

| Field | Type | Default | Description |
|---|---|---|---|
| `ls_preset` | string | `""` | Named preset: `"plant"`, `"dragon"`, `"sierpinski"`, `"hilbert"`, `"tree"`.  Fills `ls_axiom`, `ls_rules`, `ls_iterations`, `ls_angle`. |
| `ls_axiom` | string | `"F"` | Initial axiom string. |
| `ls_rules` | string | `""` | Semicolon-separated production rules, e.g. `"F=FF;X=F+[[X]-X]-F[-FX]+X"`. |
| `ls_iterations` | int | `5` | Number of rule-expansion steps. |
| `ls_angle` | float | `25.0` | Turtle turn angle in degrees (`+` turns left, `-` turns right). |
| `ls_fg_color` | string | `"#33CC55"` | Foreground (line) colour as `#RRGGBB` hex. |
| `ls_bg_color` | string | `"#080D08"` | Background fill colour as `#RRGGBB` hex. |

### Viewport

Nested object that maps pixels to complex-plane coordinates.

```json
"viewport": {
  "real_min": -2.5,
  "real_max":  1.0,
  "imag_min": -1.25,
  "imag_max":  1.25
}
```

| Field | Type | Default | Description |
|---|---|---|---|
| `real_min` | float | `-2.5` | Left edge of the complex plane window. |
| `real_max` | float | `1.0` | Right edge. |
| `imag_min` | float | `-1.25` | Bottom edge. |
| `imag_max` | float | `1.25` | Top edge. |

If the viewport aspect ratio does not match `width/height` the image will be non-square stretch of the mathematical space — usually intentional for tall/wide crops.

### Palette

Nested object for the gradient used to colour escape-time / noise values.

```json
"palette": {
  "stops": [
    { "t": 0.0,  "color": "#000033" },
    { "t": 0.5,  "color": "#FF8800" },
    { "t": 1.0,  "color": "#FFFFFF" }
  ],
  "lab_interpolation": false,
  "phase": 0.0
}
```

| Field | Type | Default | Description |
|---|---|---|---|
| `stops` | array | built-in blue-gold | Array of `{ "t": float, "color": "#RRGGBB" }` entries.  `t` in [0, 1]. |
| `lab_interpolation` | bool | `false` | Interpolate in CIELAB space instead of sRGB (perceptually uniform, avoids grey mid-tones). |
| `phase` | float | `0.0` | Rotation offset applied before sampling: `t_actual = fmod(t + phase + 1, 1)`. |

### ACO palette import

```json
"aco_palette": "palettes/my_swatches.aco"
```

Path to an Adobe Photoshop `.aco` swatch file.  When present, this **overrides** the `palette` object.  Version-1 and version-2 ACO files are supported.  Supported colour spaces: RGB, HSB, CMYK, L\*a\*b\*, Grayscale.

### Post-processing

Nested object applied after the algorithm render, before saving.

```json
"postprocess": {
  "gamma":      2.2,
  "brightness": 0.0,
  "contrast":   1.0,
  "saturation": 1.0,
  "vignette":   0.0
}
```

| Field | Type | Default | Description |
|---|---|---|---|
| `gamma` | float | `1.0` | Gamma correction exponent.  `1.0` = linear, `2.2` = standard display gamma. |
| `brightness` | float | `0.0` | Additive brightness adjustment (−1 to +1). |
| `contrast` | float | `1.0` | Contrast multiplier around mid-grey.  `< 1` = lower contrast, `> 1` = higher contrast. |
| `saturation` | float | `1.0` | Saturation scale.  `0` = greyscale, `1` = unchanged, `> 1` = boosted. |
| `vignette` | float | `0.0` | Vignette strength (0–1).  Darkens corners with a quadratic radial falloff. |

---

## Minimal example

```json
{
  "algorithm": "mandelbrot",
  "width": 800,
  "height": 600,
  "output": "output/test.png"
}
```

## Full example (Burning Ship crop)

```json
{
  "algorithm": "burning_ship",
  "width": 3840,
  "height": 2160,
  "max_iterations": 2000,
  "coloring_mode": "histogram_eq",
  "color_cycle": 32.0,
  "viewport": {
    "real_min": -1.8,
    "real_max": -1.7,
    "imag_min": -0.09,
    "imag_max":  0.01
  },
  "palette": {
    "stops": [
      { "t": 0.0, "color": "#0a0010" },
      { "t": 0.4, "color": "#8b0000" },
      { "t": 0.7, "color": "#ff6600" },
      { "t": 1.0, "color": "#ffe0a0" }
    ],
    "lab_interpolation": true
  },
  "postprocess": {
    "gamma": 2.0,
    "contrast": 1.15,
    "vignette": 0.3
  },
  "output": "output/burning_ship_crop.png",
  "output_dpi": 300,
  "aa_samples": 4,
  "threads": 0
}
```

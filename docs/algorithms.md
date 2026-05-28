# Algorithms

All algorithms are specified by the `"algorithm"` key in a scene JSON file.  Each section below lists the relevant config keys; see [config-reference.md](config-reference.md) for complete documentation of every field.

---

## Mandelbrot Set

**Key:** `"mandelbrot"` (default when key is omitted)

The classic escape-time fractal.  Each pixel maps to a complex number _c_; the iteration _zₙ₊₁ = zₙ² + c_ is applied up to `max_iterations`.  Points that never escape are in the set (rendered black by default); escaping points are coloured by their iteration count.

```json
{
  "algorithm": "mandelbrot",
  "max_iterations": 1000,
  "escape_radius": 2.0,
  "smooth_coloring": true,
  "color_cycle": 64.0,
  "coloring_mode": "smooth"
}
```

**Coloring modes**

| Mode | Description |
|---|---|
| `"smooth"` | Linas Vepstas formula — continuous colour bands, no banding artefacts |
| `"histogram_eq"` | Two-pass histogram equalisation — maximises contrast across the whole image |
| `"orbit_trap"` | Colour by minimum `|z|` seen during iteration — reveals fine detail near the boundary |

---

## Julia Set

**Key:** `"julia"`

Similar to Mandelbrot but _c_ is fixed (the seed) and _z₀_ is the pixel coordinate.  Different seeds produce wildly different shapes — from connected dendrites to totally disconnected dust.

```json
{
  "algorithm": "julia",
  "julia_cr": -0.7,
  "julia_ci":  0.27015
}
```

Notable seeds to try:

| `julia_cr` | `julia_ci` | Character |
|---|---|---|
| `-0.7` | `0.27015` | Siegel disc — spiral arms |
| `0.285` | `0.01` | Cauliflower |
| `-0.4` | `0.6` | Rabbit fractal |
| `-0.835` | `-0.2321` | San Marco |
| `0.0` | `0.8` | Near-circular |

---

## Burning Ship

**Key:** `"burning_ship"`

Variant of the Mandelbrot iteration where both components are made absolute before squaring: _zₙ₊₁ = (|Re(zₙ)| + i·|Im(zₙ)|)² + c_.  Produces a distinctive ship-like shape with jagged, flame-like boundaries.

```json
{
  "algorithm": "burning_ship",
  "viewport": {
    "real_min": -2.5, "real_max": 1.5,
    "imag_min": -2.0, "imag_max": 0.5
  }
}
```

---

## Newton Fractal

**Key:** `"newton"`

Applies Newton's method for root-finding to _f(z) = zⁿ − 1_.  Colours pixels by which root they converge to (hue) and how quickly (brightness).  Creates beautiful basin-of-attraction boundaries.

```json
{
  "algorithm": "newton",
  "newton_power": 3,
  "newton_tolerance": 1e-6,
  "newton_saturation": 0.8
}
```

`newton_power` sets _n_ (the polynomial degree, ≥ 2).  Higher powers create more roots and more intricate basins.

---

## Simplex Noise FBM

**Key:** `"noise"`

Fractal Brownian Motion built on 2D simplex noise.  Multiple octaves of noise are summed with decreasing amplitude and increasing frequency.  Produces organic cloud/marble/terrain textures that map well to any palette.

```json
{
  "algorithm": "noise",
  "noise_octaves": 6,
  "noise_persistence": 0.5,
  "noise_lacunarity": 2.0,
  "noise_scale": 1.0,
  "noise_seed": 42
}
```

| Parameter | Effect |
|---|---|
| `noise_octaves` | Number of frequency layers; more = finer detail |
| `noise_persistence` | Amplitude falloff per octave (0–1); lower = smoother |
| `noise_lacunarity` | Frequency multiplier per octave; higher = finer grain |
| `noise_scale` | Overall frequency; higher = more repetition per image |

---

## Reaction-Diffusion (Gray-Scott)

**Key:** `"reaction_diffusion"` or `"rd"`

Simulates two chemical species _U_ and _V_ diffusing and reacting on a grid according to Gray-Scott kinetics.  After `rd_steps` time steps, the concentration of _V_ is mapped through the palette.  The system is **not tileable** — the renderer always runs it as a single full-image pass.

```json
{
  "algorithm": "reaction_diffusion",
  "rd_preset": "coral",
  "rd_steps": 8000,
  "rd_seed": 42
}
```

**Named presets**

| Preset | `feed` | `kill` | Visual character |
|---|---|---|---|
| `"coral"` (default) | 0.0545 | 0.062 | Branching coral |
| `"mitosis"` | 0.0367 | 0.0649 | Cell-division spots |
| `"worms"` | 0.082 | 0.061 | Worm-like stripes |
| `"maze"` | 0.029 | 0.057 | Labyrinths |
| `"spots"` | 0.037 | 0.061 | Leopard spots |
| `"fingerprint"` | 0.037 | 0.065 | Fingerprint whorls |

Set `rd_feed` and `rd_kill` manually to explore the full Gray-Scott parameter space.

---

## L-System (Lindenmayer System)

**Key:** `"lsystem"`

A string-rewriting system that produces fractal plant and curve structures.  An axiom string is expanded by applying production rules for `ls_iterations` steps, then a turtle interpreter draws the result into the image.  The algorithm is **not tileable**.

```json
{
  "algorithm": "lsystem",
  "ls_preset": "plant",
  "ls_iterations": 6,
  "ls_angle": 25.0,
  "ls_fg_color": "#33CC55",
  "ls_bg_color": "#080D08"
}
```

**Named presets**

| Preset | Character | Default iterations | Default angle |
|---|---|---|---|
| `"plant"` | Barnsley fern variant | 6 | 25° |
| `"dragon"` | Dragon curve | 12 | 90° |
| `"sierpinski"` | Sierpiński triangle | 6 | 120° |
| `"hilbert"` | Hilbert curve | 6 | 90° |
| `"tree"` | Symmetric tree | 4 | 22.5° |

**Custom rules**

Override or extend any preset by supplying `ls_axiom` and `ls_rules`:

```json
{
  "algorithm": "lsystem",
  "ls_axiom": "F",
  "ls_rules": "F=F+F--F+F",
  "ls_iterations": 5,
  "ls_angle": 60.0
}
```

Rules are semicolon-separated `predecessor=successor` pairs.

**Turtle symbols**

| Symbol | Action |
|---|---|
| `F`, `G` | Move forward and draw a line |
| `f` | Move forward without drawing |
| `+` | Turn left by `ls_angle` degrees |
| `-` | Turn right by `ls_angle` degrees |
| `[` | Push turtle state (position + heading) |
| `]` | Pop turtle state |
| `\|` | Reverse direction (180° turn) |
| Any other letter | Ignored (production variable) |

---

## Adding Custom Algorithms

Algorithms are registered at runtime in `AlgorithmRegistry`.  To add one without modifying the core:

```cpp
#include "artgen/AlgorithmRegistry.h"
#include "artgen/config/SceneConfig.h"

struct MyAlgo : artgen::IAlgorithm {
    const char* name() const override { return "my_algo"; }
    void render(artgen::PixelBuffer& buf,
                const artgen::Viewport& vp) const override { /* … */ }
};

// Register before calling SceneConfig::create_algorithm()
artgen::AlgorithmRegistry::register_algo("my_algo",
    [](const artgen::SceneConfig&) {
        return std::make_unique<MyAlgo>();
    });
```

Then use `"algorithm": "my_algo"` in any scene JSON.

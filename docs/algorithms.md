# Algorithms

All algorithms are specified by the `"algorithm"` key in a scene JSON file.  Each
section below lists the relevant config keys; see
[config-reference.md](config-reference.md) for complete documentation of every
field.

> **Shared defaults:** many common values (`smooth_coloring`, `escape_radius`,
> `newton_tolerance`, `rd_steps`, `attractor_iterations`, `nova_relaxation`,
> `nova_max_iterations`, `physarum_num_agents`, `physarum_step_size`, etc.) are
> pre-set in [`scenes/defaults.json`](../scenes/defaults.json) and do not need to
> appear in individual scene files unless you want to override them.

---

## Table of Contents

**Escape-time family**
- [Mandelbrot Set](#mandelbrot-set)
- [Julia Set](#julia-set)
- [Burning Ship](#burning-ship)
- [Tricorn (Mandelbar)](#tricorn-mandelbar)
- [Multibrot](#multibrot)
- [Newton Fractal](#newton-fractal)
- [Nova Fractal](#nova-fractal)

**Noise & terrain**
- [Simplex Noise FBM](#simplex-noise-fbm)
- [Plasma / Diamond-Square](#plasma--diamond-square)

**Strange attractors**
- [Strange Attractor (Clifford / De Jong)](#strange-attractor-clifford--de-jong)
- [Ikeda Map Attractor](#ikeda-map-attractor)

**Spatial patterns**
- [Voronoi Diagrams](#voronoi-diagrams)
- [Reaction-Diffusion (Gray-Scott)](#reaction-diffusion-gray-scott)
- [Cyclic Cellular Automaton](#cyclic-cellular-automaton)

**Procedural & agent-based**
- [L-System (Lindenmayer System)](#l-system-lindenmayer-system)
- [Lyapunov Fractal](#lyapunov-fractal)
- [Physarum Polycephalum (Slime Mould)](#physarum-polycephalum-slime-mould)

---

## Mandelbrot Set

**Key:** `"mandelbrot"` (default when key is omitted)

The classic escape-time fractal.  Each pixel maps to a complex number _c_; the
iteration _zₙ₊₁ = zₙ² + c_ is applied up to `max_iterations`.  Points that never
escape are in the set (rendered black by default); escaping points are coloured by
their iteration count.

```json
{
  "algorithm": "mandelbrot",
  "max_iterations": 1000,
  "color_cycle": 64.0
}
```

**Coloring modes**

| Mode | Description |
|---|---|
| `"smooth"` | Linas Vepstas formula — continuous colour bands, no banding artefacts |
| `"histogram_eq"` | Two-pass histogram equalisation — maximises contrast across the whole image |
| `"orbit_trap"` | Colour by minimum `|z|` seen during iteration — reveals fine detail near the boundary |

> **⚠ Deep-zoom blank image warning**
>
> At extreme zoom depths the iteration cap is the most common cause of a completely
> black output.  For a viewport whose real (or imag) axis spans a range of **R** units,
> a rough minimum iteration count is:
>
> ```
> max_iterations ≥ max(2000,  round(500 / sqrt(R)))
> ```
>
> | Viewport range R | Minimum `max_iterations` |
> |---|---|
> | 1.0 (shallow zoom) | 2 000 |
> | 0.1 | 2 000 |
> | 0.01 | 5 000 |
> | 0.001 | 16 000 |
> | 1×10⁻⁴ | 50 000 |
> | 1×10⁻⁶ | 500 000 |
>
> A black output at deep zoom has two possible causes:
> 1. **Too few iterations** — increase `max_iterations` substantially (10× is often needed).
> 2. **Viewport is inside the set** — the target coordinate may lie in a solid black region
>    of the Mandelbrot set itself.  Verify the coordinate sits on a filament or boundary
>    feature before committing to a deep zoom.  A quick sanity check: render the full set
>    first with a crosshair overlay at the target coordinate.

---

## Julia Set

**Key:** `"julia"`

Similar to Mandelbrot but _c_ is fixed (the seed) and _z₀_ is the pixel coordinate.
Different seeds produce wildly different shapes — from connected dendrites to totally
disconnected dust.

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

Variant of the Mandelbrot iteration where both components are made absolute before
squaring: _zₙ₊₁ = (|Re(zₙ)| + i·|Im(zₙ)|)² + c_.  Produces a distinctive ship-like
shape with jagged, flame-like boundaries.

```json
{
  "algorithm": "burning_ship",
  "viewport": {
    "real_min": -2.5, "real_max": 1.5,
    "imag_min": -2.0, "imag_max": 0.5
  }
}
```

The interesting detail sits in the lower half of the complex plane; the viewport
above is flipped compared to Mandelbrot convention.

---

## Tricorn (Mandelbar)

**Key:** `"tricorn"`

Uses the conjugate of z before squaring: _zₙ₊₁ = conj(zₙ)² + c_.  The conjugation
reverses orientation at each step (antiholomorphic map), producing a tricorn-shaped
set with three-fold symmetry rather than the Mandelbrot cardioid.

```json
{
  "algorithm": "tricorn",
  "max_iterations": 1000,
  "palette": "ice"
}
```

Accepts all standard escape-time parameters (`max_iterations`, `escape_radius`,
`coloring_mode`, `color_cycle`, `palette`).

---

## Multibrot

**Key:** `"multibrot"`

Generalisation of Mandelbrot to arbitrary exponents: _zₙ₊₁ = zₙᵈ + c_.  Uses polar
form internally so non-integer exponents are valid.

```json
{
  "algorithm": "multibrot",
  "multibrot_power": 4.0
}
```

`multibrot_power` (default 3.0) — at 2.0 this exactly reproduces the Mandelbrot set.
Integer values produce (d−1)-fold rotational symmetry; non-integers break symmetry
subtly.  Accepts all standard escape-time parameters.

---

## Newton Fractal

**Key:** `"newton"`

Applies Newton's method for root-finding to _f(z) = zⁿ − 1_.  Colours pixels by
which root they converge to (hue) and how quickly (brightness).  Creates beautiful
basin-of-attraction boundaries.

```json
{
  "algorithm": "newton",
  "newton_power": 3,
  "newton_tolerance": 1e-6,
  "newton_saturation": 0.8
}
```

`newton_power` sets _n_ (polynomial degree, ≥ 2).  Higher powers create more roots
and more intricate basins.  Coloring is entirely HSV-based — no palette is used.

---

## Nova Fractal

**Key:** `"nova"`

A hybrid of Newton's method and the Mandelbrot family.  Each step applies a relaxed
Newton correction and then injects the parameter _c_:

```
z' = z − R·(z^p − 1)/(p·z^{p-1}) + c
```

Converged pixels are coloured by Newton basin hue; escaped pixels are coloured by
smooth escape time via the palette.

```json
{
  "algorithm": "nova",
  "nova_type": "mandelbrot",
  "nova_power": 3,
  "nova_saturation": 0.8,
  "palette": "fire"
}
```

| Parameter | Default | Notes |
|---|---|---|
| `nova_type` | `"mandelbrot"` | `"mandelbrot"` (z₀=1, c=pixel) or `"julia"` (z₀=pixel, c=seed) |
| `nova_power` | 3 | Polynomial degree; higher = more roots, more intricate basins |
| `nova_relaxation` | 1.0 | Step-size multiplier R; values ≠ 1 create extra boundary detail. (default in `defaults.json`) |
| `nova_seed_r`, `nova_seed_i` | 1.0, 0.0 | Fixed c for Julia-type mode |
| `nova_max_iterations` | 256 | Per-pixel iteration cap. (default in `defaults.json`) |
| `nova_tolerance` | 1e-6 | Convergence tolerance. (default in `defaults.json`) |
| `nova_escape_radius` | 1e6 | Escape threshold. (default in `defaults.json`) |
| `nova_saturation` | 0.8 | HSV saturation for converged basin colouring |

Tileable — each pixel is independent.

> **⚠ Flat / blank image warning**
>
> Two failure modes specific to Nova:
>
> **1. Flat uniform colour (Nova Julia mode)**
> When `nova_type = "julia"`, the choice of seed (nova_seed_r, nova_seed_i) can cause
> all starting points in the viewport to converge to the same Newton basin, producing a
> single-hue flat image.  This occurs when:
> - The seed magnitude `|c| = sqrt(nova_seed_r² + nova_seed_i²)` is small (< 0.5) —
>   the Newton correction dominates and overrides all initial-position information.
> - The failing example was `(nova_seed_r=0.4, nova_seed_i=-0.6)` with `nova_power=4`,
>   giving `|c| ≈ 0.72` — borderline small, and the seed happened to sit in a direction
>   that collapsed all basins into one.
>
> **Safe seed selection for Julia mode:**
> - Choose seeds with `|c| ∈ [0.8, 2.5]` for the most varied basin structure.
> - Seeds offset in the direction of one of the roots of z^n = 1
>   (i.e. `nova_seed_r = cos(2πk/n)`, `nova_seed_i = sin(2πk/n)` for integer k)
>   reliably break basin symmetry.
> - If the image is flat, try rotating the seed by 45° or increasing `|c|` by 0.3–0.5.
>
> **2. Near-black (Nova Mandelbrot or Julia mode)**
> - `nova_power < 2` — the polynomial z^p − 1 has no root structure below degree 2.
> - `nova_type = "julia"` with `nova_max_iterations < 128` — most pixels exhaust the
>   cap without converging; increase to 512+ for Julia mode.

> **Render time note:** Julia mode (`nova_type = "julia"`) is significantly slower
> than Mandelbrot mode.  Because _z₀_ varies per pixel and can start far from all
> roots, many pixels run all the way to `nova_max_iterations` before escaping.
> Increase `nova_max_iterations` to 512+ for Julia mode to reveal full basin detail;
> expect render times ~10–30× longer than Mandelbrot mode at the same iteration cap.

---

## Simplex Noise FBM

**Key:** `"noise"`

Fractal Brownian Motion built on 2D simplex noise.  Multiple octaves of noise are
summed with decreasing amplitude and increasing frequency.  Produces organic
cloud/marble/terrain textures that map well to any palette.

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

## Plasma / Diamond-Square

**Key:** `"plasma"`

Midpoint-displacement terrain generation (diamond-square algorithm).  Builds a
heightfield grid then bilinearly samples it into the output image.  Produces organic
cloud, marble, and terrain-like textures.

```json
{
  "algorithm": "plasma",
  "plasma_roughness": 0.5,
  "plasma_octaves": 8,
  "plasma_seed": 42
}
```

| Parameter | Default | Notes |
|---|---|---|
| `plasma_roughness` | 0.5 | Amplitude scale per level [0, 1]; lower = smoother terrain |
| `plasma_octaves` | 8 | Grid resolution = 2^octaves + 1; 8 → 257×257 grid |
| `plasma_seed` | 42 | RNG seed |

**Not tileable** — the global grid cannot be split.

---

## Strange Attractor (Clifford / De Jong)

**Key:** `"attractor"`

Renders a strange attractor as an orbit-density histogram.  A single trajectory is
iterated for millions of steps; each visited pixel accumulates a density count.  The
log-normalised density is mapped through the palette.

```json
{
  "algorithm": "attractor",
  "attractor_type": "clifford",
  "attractor_a": -1.4,
  "attractor_b":  1.6,
  "attractor_c":  1.0,
  "attractor_d":  0.7,
  "palette": "fire"
}
```

`attractor_iterations` defaults to 8 000 000 (set in `defaults.json`); override here only if you need a different count.

| `attractor_type` | Equations |
|---|---|
| `"clifford"` | x′ = sin(ay) + c·cos(ax),  y′ = sin(bx) + d·cos(by) |
| `"dejong"` | x′ = sin(ay) − cos(bx),  y′ = sin(cx) − cos(dy) |

> **⚠ Blank image warning — degenerate parameter combinations**
>
> Not every combination of (a, b, c, d) produces a strange attractor.  Some combinations
> cause the orbit to collapse onto a fixed point or a small periodic cycle; the density
> histogram is then effectively empty and the output is near-black.
>
> **Clifford attractor** — empirically problematic patterns:
> - `|b| < 1.0` combined with `|d| > 1.5` — the y-component has weak coupling in but
>   strong self-feedback, causing rapid orbit collapse.  The failing example was
>   `a=-1.9, b=0.5, c=-1.0, d=2.0`.
> - Any single parameter with magnitude > 2.4 while the others are small (< 0.5).
>
> **Reliable ranges** (based on the existing working scenes):
> - All four parameters in **[−2.0, 2.0]**, with at least three of them having |value| > 1.0.
> - If `|b| < 1.0`, keep `|d| ≤ 1.2` to maintain y-channel mixing.
>
> **De Jong attractor** — more tolerant; most combinations in [−3, 3] × [−3, 3] produce
> visible orbits, but values near (a,b,c,d) = (0, 0, 0, 0) produce a degenerate orbit.
>
> If the output is near-black, try widening the viewport first (e.g. to ±4 in both axes)
> before changing parameters — occasionally the attractor is simply located outside the
> default ±3 viewport.

**Not tileable** — the full orbit must be accumulated globally.

---

## Ikeda Map Attractor

**Key:** `"ikeda"`

A strange attractor modelling light bouncing inside a nonlinear optical cavity.
Uses the same log-normalised density histogram pipeline as `"attractor"`.  Produces
sharp, geometrically folded ribbons — phase-space stretching and folding made visible.

```json
{
  "algorithm": "ikeda",
  "attractor_u": 0.9,
  "palette": "fire"
}
```

`attractor_iterations` defaults to 8 000 000 (set in `defaults.json`).

`attractor_u` (default 0.9) is the nonlinearity parameter.

| `attractor_u` | Behaviour |
|---|---|
| ≤ 0.6 | Periodic orbits — the trajectory settles onto a limit cycle.  Output is near-black. |
| 0.7–0.8 | Weakly chaotic — smaller, less expansive attractor structure |
| 0.80–0.90 | Richly chaotic — complex folded sheets; best visual results |
| > 0.90 | Periodic orbit windows re-appear — output becomes sparse or near-black |

**Recommended range:** `attractor_u` ∈ [0.75, 0.90].  The chaotic window closes sharply above 0.90; u=0.9 is the sweet spot for the densest, most visually striking attractor.

**Not tileable** — global orbit accumulation.

---

## Voronoi Diagrams

**Key:** `"voronoi"`

Partitions the canvas into cells based on nearest-seed distance.  Supports three
rendering modes and three distance metrics.

```json
{
  "algorithm": "voronoi",
  "voronoi_seeds": 32,
  "voronoi_mode": "cells",
  "voronoi_metric": "euclidean",
  "voronoi_seed": 42
}
```

**Rendering modes**

| `voronoi_mode` | Description |
|---|---|
| `"cells"` | Each cell gets a distinct colour hashed from its seed index |
| `"distance"` | Colour by normalised distance to nearest seed — radial gradients |
| `"edge"` | Bright at cell boundaries where d₁ ≈ d₂ — reveals the diagram's edge structure |

**Distance metrics**

| `voronoi_metric` | Cell shape |
|---|---|
| `"euclidean"` | Polygonal / circular — standard Voronoi |
| `"manhattan"` | Axis-aligned diamond cells |
| `"chebyshev"` | Axis-aligned square cells |

---

## Reaction-Diffusion (Gray-Scott)

**Key:** `"reaction_diffusion"` or `"rd"`

Simulates two chemical species _U_ and _V_ diffusing and reacting on a grid
according to Gray-Scott kinetics.  After `rd_steps` time steps, the concentration
of _V_ is mapped through the palette.

```json
{
  "algorithm": "reaction_diffusion",
  "rd_preset": "coral",
  "rd_seed": 42
}
```

`rd_steps` defaults to 8000 (set in `defaults.json`).

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

**Not tileable** — global simulation grid.

---

## Cyclic Cellular Automaton

**Key:** `"cyclic_ca"`

A 2-D cellular automaton where each cell holds an integer state in `[0, N−1]`.  A
cell in state _c_ advances to _(c+1) mod N_ if any neighbour already holds that
value.  Starting from random noise, the grid spontaneously organises into rotating
spirals and geometric wave fronts.

```json
{
  "algorithm": "cyclic_ca",
  "cca_states": 12,
  "cca_neighborhood": "moore",
  "cca_steps": 500,
  "cca_seed": 42
}
```

| Parameter | Default | Notes |
|---|---|---|
| `cca_states` | 12 | Number of distinct states N (2–255) |
| `cca_neighborhood` | `"moore"` | `"moore"` (8-neighbor) or `"vonneumann"` (4-neighbor) |
| `cca_steps` | 500 | Simulation steps before rendering |
| `cca_seed` | 42 | RNG seed for the initial random grid |

**Neighbourhood effects**

| Neighborhood | Character |
|---|---|
| `"moore"` (8-way) | Square, geometric crystal structures |
| `"vonneumann"` (4-way) | Diamond-shaped, smooth circular wave fronts |

**State count effects**

| N | Behaviour |
|---|---|
| 3–8 | Aggressive waves, fast lock-in, vivid contrast |
| 10–14 | Moderate complexity, clear spirals |
| 16–20 | Many competing wave fronts, slower self-organisation; needs more steps |

> **Render time note:** Each step updates every pixel, so render time scales with
> `cca_steps × width × height`.  Higher N values also tend to produce highly ordered
> final states that compress very well as PNG — a small file size is not a sign of an
> empty image, just a well-structured one.

**Not tileable** — global grid state.

---

## L-System (Lindenmayer System)

**Key:** `"lsystem"`

A string-rewriting system that produces fractal plant and curve structures.  An
axiom string is expanded by applying production rules for `ls_iterations` steps, then
a turtle interpreter draws the result into the image.

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
| `"dragon"` | Heighway dragon curve | 12 | 90° |
| `"sierpinski"` | Sierpiński triangle | 6 | 120° |
| `"hilbert"` | Hilbert space-filling curve | 6 | 90° |
| `"tree"` | Symmetric branching tree | 4 | 22.5° |

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

**Not tileable** — the full expanded string must be processed globally to determine
scale and centering.

---

## Lyapunov Fractal

**Key:** `"lyapunov"`

Maps each pixel to a parameter pair (a, b) via the viewport and evaluates the
Lyapunov exponent λ of a logistic map driven by an alternating sequence of those
two parameters.  Stable regions (λ < 0) are coloured; chaotic regions (λ ≥ 0)
fall at the dark end of the palette.

```json
{
  "algorithm": "lyapunov",
  "lyapunov_sequence": "AABAB",
  "viewport": { "real_min": 2.0, "real_max": 4.0, "imag_min": 2.0, "imag_max": 4.0 },
  "palette": "ice"
}
```

`lyapunov_warmup` (200) and `lyapunov_iterations` (1000) are set in `defaults.json`.

| Parameter | Default | Notes |
|---|---|---|
| `lyapunov_sequence` | `"AB"` | Sequence of `'A'` (use real coord) and `'B'` (use imag coord) |
| `lyapunov_warmup` | 200 | Logistic-map steps discarded before λ accumulation |
| `lyapunov_iterations` | 1000 | Steps used to compute λ |
| `lyapunov_seed_x` | 0.5 | Initial x₀ for the logistic map |

**Viewport:** keep real and imag axes within **(2, 4)** — outside this range the
logistic map diverges and produces a blank image.  Square viewports with equal real
and imag spans give the most symmetrical results.

**Sequence effects:** the sequence string completely changes the shape of the stable
regions even at the same viewport.  Try `"AB"`, `"AABAB"`, `"BABABAB"`, `"AAABBB"`,
or `"ABBAAB"`.  Longer sequences produce finer detail and take proportionally longer
to render.

> **Output note:** Lyapunov images are inherently detail-rich — expect 2–3 MB PNGs
> at 1920×1920.  The stable/chaotic boundary is fractal, so more iterations
> (`lyapunov_iterations`) sharpens the boundary at the cost of render time.

Tileable — each pixel is computed independently.

---

## Physarum Polycephalum (Slime Mould)

**Key:** `"physarum"`

Agent-based simulation of *Physarum polycephalum* foraging.  Agents deposit chemical
trails on a shared grid; trails diffuse and evaporate each step.  The emergent
networks resemble biological vascular and mycelium structures.

```json
{
  "algorithm": "physarum",
  "physarum_steps": 400,
  "physarum_sensor_angle": 45.0,
  "physarum_sensor_dist": 9.0,
  "physarum_rotation_angle": 45.0,
  "physarum_seed": 42,
  "palette": "electric"
}
```

`physarum_num_agents` (200 000), `physarum_step_size` (1.0), `physarum_deposit` (5.0), and `physarum_decay` (0.95) are set in `defaults.json`.

| Parameter | Default | Effect |
|---|---|---|
| `physarum_num_agents` | 200 000 | More agents → denser, more uniform coverage |
| `physarum_steps` | 400 | Simulation steps; more steps → more evolved networks |
| `physarum_sensor_angle` | 45° | Wider → branchy coral networks; narrower → straight highways |
| `physarum_sensor_dist` | 9 px | Longer → broad sweeping paths |
| `physarum_rotation_angle` | 45° | Smaller → gentle curves; larger → angular turns |
| `physarum_deposit` | 5.0 | Trail strength deposited each step |
| `physarum_decay` | 0.95 | Trail evaporation factor per step; lower = shorter-lived trails |

**Sensor angle recipes**

| `physarum_sensor_angle` | Character |
|---|---|
| 22.5° | Straight tubes and bold highways |
| 45° | Balanced — default organic network |
| 67.5° | Branchy coral / mycelium structure |

> **Render time note:** Render time scales with `physarum_num_agents × physarum_steps`.
> The default (200 000 agents × 400 steps) takes ~14 s on a 1080×1080 canvas.
> Reducing `physarum_num_agents` to 50 000 with more steps gives sparser but more
> fully-evolved networks in similar wall-clock time.

**Not tileable** — global agent and trail state.

---

## Adding Custom Algorithms

Algorithms are registered at runtime in `AlgorithmRegistry`.  To add one without
modifying the core:

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

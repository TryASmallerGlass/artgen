# artgen

A C++20 command-line renderer for generative and fractal art, designed for wall and textile-scale output.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

---

## Features

**12 algorithms** across four families:

| Family | Algorithms |
|---|---|
| Escape-time | Mandelbrot, Julia Set, Burning Ship, Newton Fractal, Multibrot (`z^n+c`), Tricorn |
| Generative texture | Simplex Noise FBM, Reaction-Diffusion (Gray-Scott), Plasma (Diamond-Square) |
| Structural | L-Systems (5 presets + custom rules), Strange Attractor (Clifford / De Jong) |
| Geometric | Voronoi / Worley (3 modes, 3 distance metrics) |

**Rendering**
- Multithreaded tile renderer — configurable thread count, tile size, AA supersampling (1×–16×)
- Three coloring modes for escape-time algorithms: smooth (Linas Vepstas), histogram equalisation, orbit trap
- Palette phase cycling with `--animate` to produce colour-shift frame sequences

**Palette system**
- 5 built-in palettes (`classic_mandelbrot`, `fire`, `ice`, `electric`, `grayscale`)
- LAB-space interpolation option for perceptually uniform gradients
- Generated palettes: complementary, triadic, analogous, split-complementary
- Custom gradient stops in JSON
- Adobe `.aco` colour swatch import

**Output**
- 8-bit PNG and 16-bit TIFF with DPI metadata
- 32-bit float EXR (full HDR; optional fp16 mode)
- `--sweep` — parameter sweep over any scene key → numbered frame sequence
- `--animate` — palette phase cycling shorthand
- `--preview` — SDL2 interactive window (pan, zoom, live re-render)

**Extensibility**
- `AlgorithmRegistry` plugin API — register custom algorithms at runtime with no core changes
- `--list-algorithms` prints all registered algorithm names

---

## Building

**Requirements:** CMake 3.24+, a C++20 compiler (MSVC 19.40+ or GCC 13+/Clang 16+).  
All dependencies are fetched automatically by CMake: nlohmann/json, GLM, Catch2, stb, tinyexr, SDL2.

```bash
cmake -B build -G "Visual Studio 18 2026" -A x64   # Windows (VS 2026)
# or: cmake -B build                                 # Linux / macOS
cmake --build build --config Release
```

To build without the SDL2 preview window:

```bash
cmake -B build -DARTGEN_PREVIEW=OFF
cmake --build build --config Release
```

---

## Quick start

```bash
# Render a scene from a JSON config file
./build/Release/artgen --config scenes/mandelbrot_default.json

# List all registered algorithms
./build/Release/artgen --list-algorithms

# Open the SDL2 preview window after rendering
./build/Release/artgen --config scenes/julia_default.json --preview

# Sweep a parameter across frames
./build/Release/artgen --config scenes/julia_default.json --sweep julia_ci=-0.8:0.8:0.05

# Animate palette phase (20 frames)
./build/Release/artgen --config scenes/mandelbrot_fire.json --animate 0.0:0.95:0.05

# Override parameters without editing the JSON file
./build/Release/artgen --config scenes/mandelbrot_default.json --set max_iterations=5000 --set palette=fire

# List available presets (palettes, rd_preset, ls_preset values)
./build/Release/artgen --list-presets
```

---

## Algorithms

| `"algorithm"` key | Name | Notes |
|---|---|---|
| `"mandelbrot"` | Mandelbrot Set | Classic z²+c escape-time |
| `"julia"` | Julia Set | Fixed seed `(julia_cr, julia_ci)` |
| `"burning_ship"` | Burning Ship | `(\|Re z\|+i\|Im z\|)²+c` |
| `"newton"` | Newton Fractal | Root-finds zⁿ−1; hue = basin, brightness = speed |
| `"multibrot"` | Multibrot | `z^n+c` for arbitrary float `multibrot_power` (default 3) |
| `"tricorn"` | Tricorn / Mandelbar | `conj(z)²+c`; 3-lobed with spiky tips |
| `"noise"` | Simplex Noise FBM | Fractal Brownian Motion, 2D simplex basis |
| `"reaction_diffusion"` / `"rd"` | Reaction-Diffusion | Gray-Scott model; 6 named presets |
| `"plasma"` | Plasma | Diamond-Square midpoint displacement |
| `"lsystem"` | L-System | String rewriting + turtle graphics; 5 presets |
| `"attractor"` | Strange Attractor | Clifford/De Jong orbit density histogram |
| `"voronoi"` | Voronoi | Nearest-seed coloring; 3 modes, 3 metrics |

---

## Scene config reference

All fields are optional — only include what you want to override.

```jsonc
{
  // ── Core ──────────────────────────────────────────────────────────────────
  "algorithm":  "mandelbrot",   // see Algorithms table above
  "output":     "out.png",      // .png | .tiff | .tif | .exr
  "bit_depth":  8,              // 8 or 16 (TIFF only); EXR is always 32-bit float
  "dpi":        300,
  "viewport": {
    "width": 1920, "height": 1080,
    "real_min": -2.5, "real_max": 1.0,
    "imag_min": -1.25, "imag_max": 1.25
  },

  // ── Renderer ──────────────────────────────────────────────────────────────
  "threads":   0,               // 0 = hardware_concurrency
  "tile_size": 64,
  "aa":        1,               // 1 = off | 4 = 2×2 spp | 9 = 3×3 | 16 = 4×4

  // ── Escape-time (mandelbrot, julia, burning_ship, multibrot, tricorn) ─────
  "max_iterations": 1000,
  "escape_radius":  2.0,
  "coloring_mode":  "smooth",   // "smooth" | "histogram_eq" | "orbit_trap"
  "color_cycle":    64.0,       // palette repetitions across iteration range

  // ── Multibrot ─────────────────────────────────────────────────────────────
  "multibrot_power": 3.0,       // exponent n in z^n + c (float; default 3)

  // ── Julia seed ────────────────────────────────────────────────────────────
  "julia_cr": -0.7,
  "julia_ci":  0.27015,

  // ── Newton ────────────────────────────────────────────────────────────────
  "newton_power":      3,
  "newton_tolerance":  1e-6,
  "newton_saturation": 0.8,

  // ── Simplex Noise FBM ─────────────────────────────────────────────────────
  "noise_octaves":     6,
  "noise_persistence": 0.5,
  "noise_lacunarity":  2.0,
  "noise_scale":       1.0,
  "noise_seed":        42,

  // ── Reaction-Diffusion (Gray-Scott) ───────────────────────────────────────
  "rd_preset": "coral",         // "coral" | "mitosis" | "worms" | "maze" |
                                // "spots" | "fingerprint"
  "rd_feed":   0.0545,          // overrides preset
  "rd_kill":   0.062,           // overrides preset
  "rd_steps":  8000,
  "rd_seed":   42,

  // ── Plasma (Diamond-Square) ───────────────────────────────────────────────
  "plasma_roughness": 0.5,      // amplitude falloff per level [0, 1]
  "plasma_octaves":   8,        // grid size = 2^octaves + 1  (8 → 257×257)
  "plasma_seed":      42,

  // ── Strange Attractor ─────────────────────────────────────────────────────
  "attractor_type":       "clifford", // "clifford" | "dejong"
  "attractor_a":          -1.4,
  "attractor_b":           1.6,
  "attractor_c":           1.0,
  "attractor_d":           0.7,
  "attractor_iterations":  5000000,

  // ── Voronoi ───────────────────────────────────────────────────────────────
  "voronoi_seeds":  32,
  "voronoi_seed":   42,
  "voronoi_mode":   "cells",    // "cells" | "distance" | "edge"
  "voronoi_metric": "euclidean",// "euclidean" | "manhattan" | "chebyshev"

  // ── L-System ──────────────────────────────────────────────────────────────
  "ls_preset":     "plant",     // "plant" | "dragon" | "sierpinski" |
                                // "hilbert" | "tree"
  "ls_axiom":      "X",         // override preset axiom
  "ls_rules":      "X=F+[[X]-X];F=FF", // semicolon-separated predecessor=successor
  "ls_iterations": 6,
  "ls_angle":      25.0,        // turtle turn angle in degrees
  "ls_fg_color":   "#33CC55",
  "ls_bg_color":   "#080D08",

  // ── Palette ───────────────────────────────────────────────────────────────
  "palette":       "classic_mandelbrot", // "fire" | "ice" | "electric" | "grayscale"
  "palette_lab":   false,       // interpolate in CIELAB space
  "palette_phase": 0.0,         // cyclic hue rotation [0, 1)
  "palette_type":  "named",     // "complementary" | "triadic" | "analogous" |
                                // "split_complementary" | "custom"
  "palette_hsl":   [200, 0.8, 0.5], // base HSL for generated palettes
  "palette_stops": [            // custom gradient (requires "palette_type": "custom")
    { "pos": 0.0, "color": "#000000" },
    { "pos": 0.5, "color": "#FF6600" },
    { "pos": 1.0, "color": "#FFFFFF" }
  ],
  "aco_palette": "path/to/swatches.aco", // Adobe .aco import; overrides palette_*

  // ── Post-processing ───────────────────────────────────────────────────────
  "postprocess": {
    "gamma":      1.0,          // applied first; 2.2 = standard display gamma
    "brightness": 0.0,          // additive offset [-1, 1]
    "contrast":   1.0,          // multiplier around 0.5
    "saturation": 1.0,          // 0 = greyscale, >1 = vivid
    "vignette":   0.0           // radial corner darkening [0, 1]
  }
}
```

---

## Algorithm notes

### Strange Attractor — recommended viewports

The attractor orbit has a natural coordinate range that depends on the parameters.
Set the viewport to enclose the attractor:

| Type | Typical range |
|---|---|
| Clifford (default params) | `real_min: -2.5, real_max: 2.5, imag_min: -2.5, imag_max: 2.5` |
| De Jong (classic params) | `real_min: -3.0, real_max: 3.0, imag_min: -3.0, imag_max: 3.0` |

### Voronoi modes

| `voronoi_mode` | Description |
|---|---|
| `"cells"` | Each cell gets a distinct hue from the palette (hash of seed index) |
| `"distance"` | Pixel brightness = normalised distance to nearest seed |
| `"edge"` | Bright at Voronoi boundaries; dark in cell interiors |

### L-System turtle symbols

| Symbol | Action |
|---|---|
| `F`, `G` | Move forward and draw a line |
| `f` | Move forward without drawing |
| `+` | Turn left by `ls_angle` |
| `-` | Turn right by `ls_angle` |
| `[` | Push turtle state (position + heading) |
| `]` | Pop turtle state |
| `\|` | U-turn (180°) |

---

## Preview window controls

Launch with `--preview` after a single-frame render (requires `ARTGEN_PREVIEW=ON`).

| Input | Action |
|---|---|
| Left-drag | Pan viewport |
| Scroll wheel | Zoom toward cursor |
| `←` / `→` | Shift palette phase ±0.05 and re-render |
| `R` | Re-render at current viewport |
| `S` | Save current frame as `preview_save.png` |
| `V` | Print viewport JSON to stdout; save `preview_viewport.json` |
| `Q` / `Escape` | Close window |

---

## Sweep and animate

```bash
# Sweep any scalar scene parameter over a range
# Produces:  basename_0000.ext, basename_0001.ext, …
--sweep key=start:end:step

# Supported keys:
#   max_iterations  color_cycle  escape_radius
#   julia_cr  julia_ci  noise_scale  noise_seed
#   rd_feed  rd_kill  palette_phase

# Palette phase animation (shorthand; no key= prefix needed)
--animate 0.0:0.95:0.05
```

---

## Inline overrides

Override supported scene parameters from the command line without editing JSON.
Multiple `--set` flags are applied in order after the config is loaded.

```bash
# Crank iterations on an existing scene
./build/Release/artgen --config scenes/mandelbrot_default.json --set max_iterations=5000

# Switch algorithm and write an EXR for HDR compositing
./build/Release/artgen --config scenes/mandelbrot_default.json \
  --set algorithm=julia --set julia_ci=0.156 --set output=julia.exr

# Change palette and open the preview
./build/Release/artgen --config scenes/newton_z3.json --set palette=fire --preview
```

**Settable string keys:** `algorithm`, `output`, `palette`, `coloring_mode`,
`rd_preset`, `ls_preset`, `ls_fg_color`, `ls_bg_color`

**Settable numeric keys:** `max_iterations`, `color_cycle`, `escape_radius`,
`julia_cr`, `julia_ci`, `noise_scale`, `noise_seed`, `noise_octaves`,
`rd_feed`, `rd_kill`, `rd_steps`, `palette_phase`,
`aa`, `threads`, `dpi`, `bit_depth`,
`newton_power`, `ls_iterations`, `ls_angle`

---

## Custom algorithms

Register an algorithm at runtime without touching the core:

```cpp
#include "artgen/AlgorithmRegistry.h"
#include "artgen/config/SceneConfig.h"

struct MyAlgo : artgen::IAlgorithm {
    const char* name() const override { return "my_algo"; }
    void render(artgen::PixelBuffer& buf,
                const artgen::Viewport& vp) const override { /* … */ }
};

// Call before SceneConfig::create_algorithm()
artgen::AlgorithmRegistry::register_algo("my_algo",
    [](const artgen::SceneConfig&) {
        return std::make_unique<MyAlgo>();
    });
```

Then use `"algorithm": "my_algo"` in any scene JSON.

---

## Running tests

```bash
ctest --test-dir build -C Release
# 119/119 tests pass
```

---

## Project structure

```
artgen/
├── CMakeLists.txt
├── docs/
│   ├── algorithms.md           # per-algorithm reference with JSON examples
│   └── config-reference.md     # complete config field reference
├── include/artgen/
│   ├── IAlgorithm.h            # algorithm interface
│   ├── AlgorithmRegistry.h     # runtime plugin factory
│   ├── AcoImport.h             # Adobe .aco palette import
│   ├── PixelBuffer.h           # 8/16-bit RGBA buffer
│   ├── Viewport.h              # complex-plane bounding box
│   ├── Palette.h               # gradient with LAB interpolation + phase
│   ├── PaletteGenerator.h      # generated palette schemes
│   ├── PostProcess.h           # gamma / contrast / vignette stack
│   ├── Renderer.h              # multithreaded tile renderer
│   ├── Preview.h               # SDL2 interactive window (optional)
│   ├── Color.h                 # RGB/HSL/HSV/LAB/CMYK conversions
│   ├── algorithms/             # one header per algorithm
│   ├── config/SceneConfig.h    # JSON scene loader + algorithm factory
│   └── output/                 # PNG, TIFF, EXR writer headers
├── src/
│   ├── algorithms/             # algorithm implementations
│   ├── core/                   # renderer, palette, registry, post-process …
│   ├── config/                 # JSON parsing
│   └── output/                 # stb PNG, TIFF, tinyexr EXR
├── tests/                      # Catch2 unit tests (119 cases)
└── scenes/                     # example JSON scene configs
```

---

## License

MIT — see [LICENSE](LICENSE).

# artgen

A C++20 command-line renderer for generative and fractal art, designed for wall and textile-scale output.

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## Features

- **Fractal algorithms** — Mandelbrot, Julia Set, Burning Ship, Newton Fractal
- **Organic algorithms** — Simplex Noise FBM, Reaction-Diffusion (Gray-Scott), L-Systems (5 presets)
- **Coloring modes** — smooth (Linas Vepstas), histogram equalisation, orbit trap
- **Palette system** — 5 built-in palettes, LAB-space interpolation, CMYK support, generated palettes (complementary/triadic/analogous/split-complementary), custom JSON stops, cyclic phase offset
- **High-res output** — 8-bit PNG, 16-bit TIFF with DPI metadata, and 32-bit float OpenEXR; 4K in ~2.4 s on 16 threads
- **Post-processing** — gamma, contrast, brightness, saturation, vignette
- **Multithreaded tile renderer** — configurable thread count, tile size, supersampling AA (1×–4×)
- **Batch modes** — `--sweep` (parameter animation), `--animate` (palette phase cycling)
- **Interactive preview** — SDL2 window with pan/zoom, palette cycling, live re-render (`--preview`)

## Building

**Requirements:** CMake 3.24+, a C++20 compiler (MSVC 19.40+ or GCC 13+/Clang 16+).  
Dependencies are fetched automatically by CMake: nlohmann/json, GLM, Catch2, stb, SDL2.

```bash
cmake -B build -G "Visual Studio 18 2026" -A x64   # Windows
# or: cmake -B build                                 # Linux / macOS
cmake --build build --config Release
```

To build without the SDL2 preview window:

```bash
cmake -B build -DARTGEN_PREVIEW=OFF
```

## Quick start

```bash
# Render a scene from a JSON config file
./build/Release/artgen --config scenes/mandelbrot_default.json

# Open the SDL2 preview window after rendering
./build/Release/artgen --config scenes/mandelbrot_default.json --preview

# Override parameters without editing the JSON file
./build/Release/artgen --config scenes/mandelbrot_default.json --set max_iterations=5000 --set palette=fire

# Sweep a parameter across frames (e.g. zoom in)
./build/Release/artgen --config scenes/julia_default.json --sweep julia_ci=0.0:0.5:0.05

# Export a palette-cycling animation (20 frames)
./build/Release/artgen --config scenes/mandelbrot_fire.json --animate 0.0:0.95:0.05

# Discover available algorithms and presets
./build/Release/artgen --list-algorithms
./build/Release/artgen --list-presets
```

## Scene config reference

Scenes are JSON files in `scenes/`. All fields are optional and fall back to defaults.

```jsonc
{
  // ── Core ──────────────────────────────────────────────────────────────────
  "algorithm":    "mandelbrot",    // see Algorithms below
  "output":       "out.png",       // .png | .tiff / .tif | .exr (32-bit float)
  "bit_depth":    8,               // 8 or 16 (16 only meaningful for TIFF)
  "viewport": {
    "width": 1920, "height": 1080,
    "real_min": -2.5, "real_max": 1.0,
    "imag_min": -1.25, "imag_max": 1.25
  },

  // ── Escape-time (mandelbrot, julia, burning_ship) ─────────────────────────
  "max_iterations": 1000,
  "escape_radius":  2.0,
  "smooth_coloring": true,
  "color_cycle":    64.0,
  "coloring_mode":  "smooth",      // "smooth" | "histogram_eq" | "orbit_trap"

  // ── Palette ───────────────────────────────────────────────────────────────
  "palette":        "classic_mandelbrot", // "fire" | "ice" | "electric" | "grayscale"
  "palette_lab":    false,         // interpolate in LAB colour space
  "palette_phase":  0.0,           // cyclic hue shift [0, 1)
  "palette_type":   "named",       // "complementary" | "triadic" | "analogous" |
                                   // "split_complementary" | "custom"
  "palette_hsl":    [200, 0.8, 0.5], // base HSL for generated palettes
  "palette_stops":  [              // custom gradient (overrides palette_type)
    { "pos": 0.0, "color": "#000000" },
    { "pos": 0.5, "color": "#FF6600" },
    { "pos": 1.0, "color": "#FFFFFF" }
  ],

  // ── Julia seed ────────────────────────────────────────────────────────────
  "julia_cr": -0.7, "julia_ci": 0.27015,

  // ── Newton ────────────────────────────────────────────────────────────────
  "newton_power": 3, "newton_tolerance": 1e-6, "newton_saturation": 0.8,

  // ── Noise (simplex FBM) ───────────────────────────────────────────────────
  "noise_octaves": 6, "noise_persistence": 0.5,
  "noise_lacunarity": 2.0, "noise_scale": 1.0, "noise_seed": 42,

  // ── Reaction-diffusion (Gray-Scott) ───────────────────────────────────────
  "rd_preset": "coral",            // "coral" | "mitosis" | "worms" | "maze" |
                                   // "spots" | "fingerprint"
  "rd_feed": 0.0545, "rd_kill": 0.062,
  "rd_Du": 0.2097, "rd_Dv": 0.1050,
  "rd_steps": 8000, "rd_seed": 42,

  // ── L-System ──────────────────────────────────────────────────────────────
  "ls_preset":     "plant",        // "plant" | "dragon" | "sierpinski" |
                                   // "hilbert" | "tree"
  "ls_axiom":      "X",            // override preset axiom
  "ls_rules":      "X=F+[[X]-X];F=FF", // override preset rules (semicolon-separated)
  "ls_iterations": 6,
  "ls_angle":      25.0,           // turtle turn angle in degrees
  "ls_fg_color":   "#33CC55",
  "ls_bg_color":   "#080D08",

  // ── Post-processing (applied after render) ────────────────────────────────
  "postprocess": {
    "gamma":      1.0,             // >1 brightens, <1 darkens
    "contrast":   1.0,             // multiplier around 0.5
    "brightness": 0.0,             // additive offset
    "saturation": 1.0,             // 0 = greyscale, >1 = vivid
    "vignette":   0.0              // 0 = off, 1 = strong radial darkening
  },

  // ── Renderer ──────────────────────────────────────────────────────────────
  "threads":   0,                  // 0 = hardware_concurrency
  "tile_size": 64,
  "aa":        1,                  // 1=off, 2=4 spp, 4=16 spp
  "dpi":       300
}
```

## Algorithms

| Name | `"algorithm"` value | Notes |
|---|---|---|
| Mandelbrot | `"mandelbrot"` | Classic z² + c escape-time |
| Julia Set | `"julia"` | Configurable seed (`julia_cr`, `julia_ci`) |
| Burning Ship | `"burning_ship"` | Absolute-value variant of Mandelbrot |
| Newton Fractal | `"newton"` | Finds roots of zⁿ − 1; HSV root colouring |
| Simplex Noise FBM | `"noise"` | Fractal Brownian Motion, 2D simplex basis |
| Reaction-Diffusion | `"reaction_diffusion"` | Gray-Scott model; 6 named presets |
| L-System | `"lsystem"` | Turtle graphics; 5 named presets |

### L-System turtle symbols

| Symbol | Action |
|---|---|
| `F`, `G` | Move forward, draw line |
| `f` | Move forward, no draw |
| `+` | Turn left by `ls_angle` |
| `-` | Turn right by `ls_angle` |
| `[` | Push turtle state |
| `]` | Pop turtle state |
| `\|` | U-turn (180°) |

## Preview window controls

Launch with `--preview` after any render.

| Input | Action |
|---|---|
| Left-drag | Pan (shift complex-plane viewport) |
| Scroll wheel | Zoom toward cursor |
| `←` / `→` | Shift palette phase ±0.05 and re-render |
| `R` | Re-render at current viewport |
| `S` | Save current frame as `preview_save.png` |
| `V` | Print viewport JSON to stdout; save `preview_viewport.json` |
| `Q` / `Escape` | Close window |

## Sweep and animate

```bash
# Sweep any scene parameter over a range → outputs basename_0000.ext, _0001.ext, …
--sweep key=start:end:step

# Supported keys: max_iterations, color_cycle, escape_radius,
#                 julia_cr, julia_ci, noise_scale, noise_seed,
#                 rd_feed, rd_kill, palette_phase

# Animate palette phase cycling (shorthand for --sweep palette_phase=…)
--animate start:end:step
```

## Inline overrides

Override any scene parameter from the command line without editing JSON.
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

## Running tests

```bash
ctest --test-dir build -C Release
# 67/67 tests pass
```

## Project structure

```
artgen/
├── CMakeLists.txt
├── include/artgen/
│   ├── IAlgorithm.h          # algorithm interface
│   ├── PixelBuffer.h         # 8/16-bit RGBA buffer
│   ├── Viewport.h            # complex-plane bounding box
│   ├── Palette.h             # gradient with LAB interpolation + phase
│   ├── PaletteGenerator.h    # complementary/triadic/analogous generators
│   ├── PostProcess.h         # post-processing stack
│   ├── Renderer.h            # multithreaded tile renderer
│   ├── Preview.h             # SDL2 interactive window
│   ├── Color.h               # RGB/HSL/HSV/LAB/XYZ/CMYK conversions
│   ├── algorithms/           # algorithm headers (incl. EscapeTime.h base class)
│   ├── config/SceneConfig.h  # JSON scene loader
│   └── output/               # PNG, TIFF, and EXR writer headers
├── src/
│   ├── algorithms/           # algorithm implementations
│   ├── core/                 # renderer, palette, post-process, preview
│   ├── config/               # scene config parser + algorithm factory
│   └── output/               # PNG (stb), TIFF, and EXR writers
├── tests/                    # Catch2 unit tests (67 cases)
└── scenes/                   # example JSON scene configs
```

## License

MIT — see [LICENSE](LICENSE).

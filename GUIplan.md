# GUI Implementation Plan

## Decisions made

- **Framework**: Dear ImGui with SDL2 backend (SDL2 already a project dependency)
- **Integration**: Same executable, launched with `--gui` flag; CMake `ARTGEN_GUI=ON/OFF` option mirroring existing `ARTGEN_PREVIEW`
- **Rendering**: Background thread with progress bar; UI stays responsive during render

---

## Architecture overview

```
artgen [--gui]
  └── main.cpp detects --gui flag → calls gui::run()
        ├── SDL2 window + SDL_Renderer (or OpenGL)
        ├── ImGui SDL2 + SDL_Renderer backend
        └── Render thread (std::thread)
              ├── Calls same AlgorithmRegistry / SceneConfig pipeline as CLI
              └── Posts progress % to atomic<int> polled by UI thread
```

The GUI links against `artgen_core` (the same static lib the CLI uses). No renderer code is duplicated.

---

## CMake changes

1. Add `option(ARTGEN_GUI "Build with Dear ImGui GUI" OFF)` to `CMakeLists.txt`.
2. When `ON`, fetch/find ImGui via `FetchContent` or vcpkg and add the SDL2 backend sources.
3. Add `ARTGEN_GUI` compile definition so `main.cpp` can conditionally include gui code.
4. SDL2 is already required by `ARTGEN_PREVIEW`; no new SDL2 setup needed.

ImGui targets to add (from the imgui source tree):
- `imgui.cpp`, `imgui_draw.cpp`, `imgui_tables.cpp`, `imgui_widgets.cpp`
- `backends/imgui_impl_sdl2.cpp`
- `backends/imgui_impl_sdlrenderer2.cpp`

---

## Prerequisite: refactor `main.cpp` helpers into `artgen_core`

Two static functions in `main.cpp` are needed by both the CLI and GUI. Move them to the core library before building the GUI:

| Function | Move to |
|---|---|
| `make_output_stem(algo_name)` | `src/core/OutputNaming.cpp` + `artgen/OutputNaming.h` |
| `save_settings_json(cfg, stem)` | same file |
| `check_render_quality(buf, algo)` | `src/core/QualityCheck.cpp` + `artgen/QualityCheck.h` |

The CLI calls them via the new header; the GUI calls them identically. No behaviour change.

---

## Progress callback: `TileRenderer` API addition

`Renderer.cpp` currently writes progress to stdout via `show_progress`. The GUI needs an in-process callback instead. Add to `TileRenderer`:

```cpp
std::function<void(int done, int total)> progress_cb; // GUI sets this
std::atomic<bool>* cancel_flag = nullptr;             // GUI sets this to abort
```

In `do_tile`, after `++done`, call `progress_cb(done, total)` if set.
At the top of each tile, check `cancel_flag && cancel_flag->load()` and return early.

**Non-tileable algorithms** (`is_tileable()` returns false: RD, Physarum, CCA, Lyapunov,
Attractor, Ikeda, LSystem): the render call blocks as one pass — no tile-level progress.
For these, the GUI shows an indeterminate spinner rather than a percentage bar.
Cancel requires a cooperative flag on `IAlgorithm`:

```cpp
// IAlgorithm.h addition
std::atomic<bool>* cancel = nullptr; // implementations check this in inner loops
```

Only the slow algorithms (RD, Physarum, CCA) need to actually poll it; attractor/lsystem
complete in under a second and don't need cancel support.

---

## File layout

```
src/
  core/
    OutputNaming.cpp   — make_output_stem, save_settings_json (moved from main.cpp)
    QualityCheck.cpp   — check_render_quality (moved from main.cpp)
  gui/
    gui.h              — gui::run() declaration
    gui.cpp            — SDL2 window loop, ImGui frame pump, RenderJob management
    panels/
      algo_panel.cpp       — algorithm selector + per-algo parameter widgets
      viewport_panel.cpp   — width/height/coord bounds inputs, aspect-lock toggle
      palette_panel.cpp    — palette picker, gradient preview bar, colour-stop editor
      postprocess_panel.cpp — gamma/contrast/brightness/saturation/vignette sliders
      output_panel.cpp     — format/bit-depth/DPI selectors, render button, progress
      preview_panel.cpp    — rendered image as SDL texture; pan/zoom interaction
    file_ops.cpp           — scene open/save, recent-files list
```

---

## UI layout (single window, resizable)

```
┌─────────────────────────────────────────────────────────────────────┐
│ File ▾   Edit ▾                                                     │
├──────────────────────┬──────────────────────────────────────────────┤
│  Algorithm [dropdown]│                                              │
│                      │   Preview                                    │
│  Parameters          │   (rendered image; drag to pan,             │
│  (per-algo fields)   │    scroll to zoom, dbl-click to recenter)   │
│                      │                                              │
│  Viewport            │   Quality warning bar (if triggered)        │
│  w / h / coords      │                                              │
│  [aspect lock ☐]    │                                              │
│                      │                                              │
│  Palette             │                                              │
│  [gradient bar ████] │                                              │
│                      │                                              │
│  Post-process ▸      │                                              │
│  (collapsible)       │                                              │
├──────────────────────┴──────────────────────────────────────────────┤
│  [Render ▶]  [Quick ⚡]  ████████████░░  67%   PNG ▼  [Cancel ✕]  │
│  Last: gallery/mand_20260618_1423_00003.png   [Open folder 📁]     │
└─────────────────────────────────────────────────────────────────────┘
```

**Menu bar:**
- `File > Open Scene…` — `SceneConfig::from_json()` into widget state
- `File > Save Scene…` — serialise widget state to a user-chosen `.json`
- `File > Recent` — last 10 `ImageSettings/*.json` files (read `.counter`)
- `Edit > Copy as JSON` — serialise current config to clipboard

---

## Quick render mode

The **[Quick ⚡]** button renders at ¼ resolution with `max_iterations / 4` (or
`rd_steps / 4`, `physarum_steps / 4`, etc.) for rapid visual exploration. The result
is upscaled to fill the preview area. This is especially valuable for RD, Physarum,
and CCA which are slow at full resolution.

No new config key is needed — the GUI constructs a temporary `SceneConfig` with the
reduced parameters and renders to a smaller `PixelBuffer`, then upscales it via SDL.

---

## Interactive viewport in the preview panel

| Gesture | Action |
|---|---|
| Left-drag | Pan (shift real/imag bounds) |
| Scroll wheel | Zoom in/out centred on cursor |
| Double-click | Centre on clicked point (reset zoom level) |
| Right-click | Context menu: "Render this view", "Copy viewport JSON" |

After any gesture, the `viewport_panel` inputs update to reflect the new bounds. The
user clicks **[Render]** to actually produce the image (no auto-render on gesture to
avoid queuing dozens of renders while panning).

---

## Per-algorithm parameter panels

Each algorithm exposes a different subset of `SceneConfig` fields. The panel function signature:

```cpp
void draw_algo_params(SceneConfig& cfg, const std::string& algo);
```

A switch/map dispatches to per-algo sub-functions, e.g. `draw_mandelbrot_params`, `draw_nova_params`, etc.

### Algorithm → fields mapping (summary)

| Algorithm | Key fields | Widget notes |
|---|---|---|
| mandelbrot, burning_ship, tricorn | max_iterations, escape_radius, smooth_coloring, coloring_mode, color_cycle | coloring_mode: radio buttons |
| julia | + julia_cr, julia_ci | drag sliders in [−2, 2] |
| multibrot | + multibrot_power | drag float, min 2.0 |
| newton | newton_power, newton_tolerance, newton_saturation | power: int slider 2–12 |
| nova | nova_type (radio), nova_power, nova_relaxation, nova_seed_r/i (only if julia), nova_max_iterations, nova_tolerance, nova_saturation | conditional seed fields |
| noise | noise_octaves, noise_persistence, noise_lacunarity, noise_scale, noise_seed | |
| reaction_diffusion | rd_preset (combo), rd_feed, rd_kill, rd_steps, rd_seed | preset combo auto-fills feed/kill; manual override enabled |
| plasma | plasma_roughness, plasma_octaves, plasma_seed | roughness: 0.0–1.0 slider |
| lsystem | ls_preset (combo), ls_axiom, ls_rules, ls_iterations, ls_angle, ls_fg_color, ls_bg_color | **ColorEdit3 for fg/bg** |
| attractor | attractor_type (radio: clifford/dejong), attractor_a–d, attractor_iterations | a–d: drag float in [−3, 3] |
| ikeda | attractor_u, attractor_iterations | u: slider 0.75–0.95 with advisory tooltip |
| voronoi | voronoi_seeds, voronoi_seed, voronoi_mode (combo), voronoi_metric (combo) | |
| cyclic_ca | cca_states, cca_neighborhood (radio), cca_steps, cca_seed | |
| lyapunov | lyapunov_sequence (text input), lyapunov_warmup, lyapunov_iterations | tooltip: "must contain both A and B" |
| physarum | physarum_num_agents, physarum_steps, physarum_sensor_angle, physarum_sensor_dist, physarum_rotation_angle, physarum_step_size, physarum_deposit, physarum_decay, physarum_seed | |

**Validation hints**: where `AlgorithmRegistry` already prints `stderr` warnings (ikeda u range,
lyapunov sequence, nova power, attractor viewport), mirror the same checks in the widget
and show an `ImGui::TextColored(yellow, ...)` advisory inline rather than waiting for a failed render.

---

## Palette panel additions

The existing plan lacks detail on the palette UI. The palette system has four distinct modes
that need different widgets:

```
Palette mode: [Named ●] [Generated ○] [Custom stops ○] [ACO file ○]

Named:      [combo: fire / ice / electric / classic_mandelbrot / grayscale]
Generated:  [combo: complementary / triadic / analogous / split_complementary]
            Hue [slider 0–360] Sat [0–1] Lum [0–1]
Custom:     [+ Add stop]  pos=0.00 [color] | pos=0.50 [color] | ...
ACO file:   [Browse…]  path shown

[████████████████████]  ← gradient preview bar (128×12 px SDL texture, updates live)

palette_phase [slider 0–1]
palette_lab   [checkbox]  "LAB interpolation"
color_cycle   [drag float]
```

The gradient bar must regenerate whenever any palette parameter changes. Call `cfg.build_palette()`
and sample it at 128 points to fill a small SDL texture.

---

## Post-process panel (collapsible)

The `SceneConfig::postprocess` struct has gamma, contrast, brightness, saturation, vignette —
none of these appear in the current plan. Expose them in a collapsible section:

```
▸ Post-process
  Gamma      [1.00 ════════▐]   0.5 – 3.0
  Contrast   [1.00 ════════▐]   0.5 – 2.0
  Brightness [0.00 ════════▐]  -0.5 – 0.5
  Saturation [1.00 ════════▐]   0.0 – 2.0
  Vignette   [0.00 ════════▐]   0.0 – 1.0
  [Reset to defaults]
```

---

## Render thread design

```cpp
struct RenderJob {
    SceneConfig              config;      // owned copy — safe to read after launch
    std::atomic<int>         progress{0}; // 0–100; -1 = indeterminate (non-tileable)
    std::atomic<bool>        done{false};
    std::atomic<bool>        cancel{false};
    std::string              output_path;
    std::string              quality_warning; // written by worker, read after done
    std::unique_ptr<artgen::PixelBuffer> result; // set by worker when done
    std::thread              thread;
};

// Owned by gui.cpp as std::unique_ptr<RenderJob>
// Never destroyed while thread is running: join before replacing.
```

**Worker thread flow:**
1. `TileRenderer::progress_cb` writes `job.progress = done * 100 / total`.
2. `TileRenderer::cancel_flag = &job.cancel`.
3. For non-tileable algos set `job.progress = -1` (spinner).
4. Quality check result stored in `job.quality_warning` (string, not stderr).
5. On completion: `job.result = std::make_unique<PixelBuffer>(buf)`, then `job.done = true`.

**Replace vs. queue:** clicking **[Render]** while a job is running sets `job.cancel = true`,
joins the thread (which should exit quickly for tileable algos; may take a moment for RD/CCA),
then starts a new job. No queue — the user's intent is always the current parameter set.

**Preview upload:** the UI thread polls `job.done` each frame. When it goes true, it uploads
`job.result` pixel data to an `SDL_Texture` via `SDL_UpdateTexture`. The texture is then
drawn via `ImGui::Image`.

---

## Output behaviour (identical to CLI)

- `make_output_stem()` and `save_settings_json()` are called from `artgen_core` (after refactor).
- Output directory is derived from the last-used scene's `output_path`, defaulting to `gallery/`.
- The GUI exposes a directory picker (or text field) for the output directory.
- Extension follows the format selector: `.png` / `.tiff` / `.exr`.

---

## Persistent GUI state

ImGui's `.ini` saves panel sizes and positions automatically. Additionally, persist the last-used
`SceneConfig` and output directory to `ImageSettings/gui_state.json` on exit and reload on start.
This means relaunching `--gui` resumes exactly where the user left off.

The easiest implementation: on exit, call `save_settings_json(current_cfg, "gui_state")`;
on startup, if `ImageSettings/gui_state.json` exists, call `SceneConfig::from_json()` on it.

---

## Quality warning display

`check_render_quality` currently writes to `stderr`. In the GUI:
1. The worker captures the warning string into `RenderJob::quality_warning`.
2. After `done`, if the string is non-empty, show a yellow banner across the bottom of the
   preview panel: `⚠ Near-black output detected. [?]` with a tooltip showing the hint text.
3. The banner is dismissed by clicking `[✕]` or by starting a new render.

---

## What is NOT in scope for the first GUI pass

- Live parameter preview on mouse-move (too slow for fractal renders)
- Sweep / animate controls (stay CLI-only for now)
- Replacing the existing SDL2 `--preview` window (it stays as-is)
- Custom L-System rule editor (preset combo only; axiom/rules still JSON-only)
- ACO palette file browser (path text field only)
- Multi-document / tabbed interface

---

## Implementation order

1. **Refactor** `make_output_stem`, `save_settings_json`, `check_render_quality` into `artgen_core`
2. **TileRenderer API** — add `progress_cb` and `cancel_flag` fields; wire into `do_tile`
3. **CMake wiring** — `ARTGEN_GUI` option, ImGui source addition, compile guard in `main.cpp`
4. **Minimal window loop** — SDL2 + ImGui init, empty frame, clean shutdown
5. **Algorithm selector + SceneConfig binding** (no render yet)
6. **Viewport and palette panels** including gradient preview bar
7. **Post-process panel**
8. **RenderJob + render thread + progress bar** (tileable algos first)
9. **Preview texture display** + pan/zoom gestures
10. **File ops** — Open Scene, Save Scene, Recent files, Copy as JSON
11. **Non-tileable algo cancel** — IAlgorithm cancel flag in RD, Physarum, CCA
12. **Quick render mode**
13. **Per-algo parameter panels** — one algo at a time; inline validation hints
14. **Persistent GUI state** — gui_state.json load/save
15. **Polish** — quality warning banner, keyboard shortcuts (F5=Render, F9=Quick, Ctrl+O/S)

---

## Open questions

1. **Interactive viewport gestures** — should drag-to-pan + scroll-to-zoom be in scope
   for the first pass, or deferred to a later phase? They're high-value for fractal exploration
   but require tracking mouse state across frames.

2. **Quick render resolution** — should the ¼-resolution quick render use a fixed downscale
   (e.g. always `512×512`), or a relative fraction of the configured output size?

3. **RD / Physarum cancel latency** — these algorithms have inner loops that run thousands
   of iterations. Cancel polling frequency determines UI responsiveness. Every 10 steps (RD)
   or every agent-step (Physarum) seems reasonable — confirm this is acceptable or
   if the user prefers a simpler "wait for finish" with no cancel for slow algos.

4. **Scene save format** — should `File > Save Scene…` write a minimal diff-from-defaults
   JSON (compact, as the existing hand-written scenes do) or the full `save_settings_json`
   dump (verbose, with all fields)? The existing ImageSettings mechanism always saves the full
   form; a "save scene" for reuse probably wants the compact form.

5. **Postprocess exposure** — is the post-process panel worth including in the first GUI
   pass, or is it low priority? The sliders are trivial to add but they add visual complexity.

6. **Output format selector** — should TIFF and EXR be available in the GUI, or PNG-only
   for the first pass (matching the most common use case)?

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

## File layout

```
src/
  gui/
    gui.h          — gui::run() declaration
    gui.cpp        — SDL2 window loop, ImGui frame pump
    panels/
      algo_panel.cpp    — algorithm selector + per-algo parameter widgets
      viewport_panel.cpp — width/height/coord bounds inputs
      palette_panel.cpp  — palette picker + colour-stop editor
      output_panel.cpp   — format/bit-depth selectors, render button, progress bar
      preview_panel.cpp  — rendered image as an SDL texture displayed in ImGui
```

---

## UI layout (single window)

```
┌─────────────────────────────────────────────────────────┐
│  Algorithm  [dropdown ▼]                                │
├──────────────────┬──────────────────────────────────────┤
│  Parameters      │                                      │
│  (per-algo       │   Preview                            │
│   fields)        │   (rendered image, updates after     │
│                  │    each completed render)            │
├──────────────────┤                                      │
│  Viewport        │                                      │
│  w/h/coords      │                                      │
├──────────────────┤                                      │
│  Palette         │                                      │
│  picker          │                                      │
├──────────────────┴──────────────────────────────────────┤
│  [Render]  ████████████░░░░░░░░  67%   Output: PNG ▼   │
│  Last: gallery/mand_20260618_1423_00003.png             │
└─────────────────────────────────────────────────────────┘
```

---

## Per-algorithm parameter panels

Each algorithm exposes a different subset of `SceneConfig` fields. The panel function signature:

```cpp
void draw_algo_params(SceneConfig& cfg, const std::string& algo);
```

A switch/map dispatches to per-algo sub-functions, e.g. `draw_mandelbrot_params`, `draw_nova_params`, etc.

### Algorithm → fields mapping (summary)

| Algorithm | Key fields |
|---|---|
| mandelbrot, julia, burning_ship, multibrot, tricorn | max_iterations, escape_radius, smooth_coloring, coloring_mode, color_cycle |
| julia | + julia_cr, julia_ci |
| multibrot | + multibrot_power |
| newton | newton_power, newton_tolerance, newton_saturation |
| nova | nova_type, nova_power, nova_relaxation, nova_seed_r/i, nova_max_iterations, nova_tolerance, nova_escape_radius, nova_saturation |
| noise | noise_octaves, noise_persistence, noise_lacunarity, noise_scale, noise_seed |
| reaction_diffusion | rd_preset, rd_feed, rd_kill, rd_steps, rd_seed |
| plasma | plasma_roughness, plasma_octaves, plasma_seed |
| lsystem | ls_preset, ls_axiom, ls_rules, ls_iterations, ls_angle, ls_fg_color, ls_bg_color |
| attractor | attractor_type, attractor_a–d, attractor_iterations |
| ikeda | attractor_u, attractor_iterations |
| voronoi | voronoi_seeds, voronoi_seed, voronoi_mode, voronoi_metric |
| cyclic_ca | cca_states, cca_neighborhood, cca_steps, cca_seed |
| lyapunov | lyapunov_sequence, lyapunov_warmup, lyapunov_iterations, lyapunov_seed_x |
| physarum | physarum_num_agents, physarum_steps, physarum_sensor_angle, physarum_sensor_dist, physarum_rotation_angle, physarum_step_size, physarum_deposit, physarum_decay, physarum_seed |

---

## Render thread design

```cpp
struct RenderJob {
    SceneConfig config;
    std::atomic<int> progress{0};   // 0–100
    std::atomic<bool> done{false};
    std::atomic<bool> cancel{false};
    std::string output_path;
};
```

- UI thread creates a `RenderJob`, spawns `std::thread(render_worker, job)`.
- `render_worker` calls the existing tile renderer; after each tile row it updates `job.progress`.
- On completion it sets `job.done = true`; UI uploads the result PNG to an SDL texture for preview.
- Cancel sets `job.cancel = true`; the tile loop checks it each tile.

Progress reporting hook: the tile renderer needs a callback/atomic pointer passed in. Check whether `AlgorithmRegistry` already supports a progress callback; if not, add one.

---

## Output behaviour (identical to CLI)

- `make_output_stem()` and `save_settings_json()` from `main.cpp` are called unchanged.
- The GUI constructs a `SceneConfig` from the widget state, sets `output_path` with directory + extension, then the same stem logic runs.
- Result: `gallery/<stem>.png` + `ImageSettings/<stem>.json` exactly as CLI.

---

## What is NOT in scope for the first GUI pass

- Live parameter preview on mouse-move (too slow for fractal renders)
- Sweep / animate controls
- Replacing the existing SDL2 `--preview` window (it stays as-is)
- Scene file open/save from the GUI (user still writes JSON for custom presets)

---

## Implementation order

1. CMake wiring — `ARTGEN_GUI` option, ImGui source addition, compile guard in `main.cpp`
2. Minimal window loop — SDL2 + ImGui init, empty frame, clean shutdown
3. Algorithm selector + SceneConfig binding (no render yet)
4. Viewport and palette panels
5. Render thread + progress bar
6. Preview texture display
7. Per-algo parameter panels (can be done one algo at a time)
8. Polish: keyboard shortcuts, window size persistence, last-used settings recall

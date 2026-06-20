# Development Principles

Design rules for extending artgen (new algorithms, new modules, GUI features). Read before adding a feature; update this file when a new rule emerges from a real decision.

## 1. CLI-first, GUI-optional

Every capability must be reachable from the command line without the GUI. The GUI is a convenience layer on top of `artgen_core`, never the only path to a feature.

- New algorithms, post-processors, and output formats are wired into `main.cpp` (`--config`, `--set key=value`, `--sweep`, `--animate`) before or alongside any GUI panel for them.
- GUI code may call into `artgen_core` / CLI-reachable functions, but core functionality must never live only inside `Gui.cpp`.
- Scriptability (batch rendering, automation, CI, headless servers) depends on this — don't trade it away for UI convenience.

## 2. Modular, plugin-style algorithms

`IAlgorithm` is the contract. Every generative technique (Mandelbrot, Julia, L-System, Reaction-Diffusion, ...) is a self-contained class behind this interface and registered in `AlgorithmRegistry`, not a branch in shared code.

- Adding an algorithm should never require editing unrelated algorithms' code.
- Shared behavior (palette application, post-processing, tiling) lives in base classes or free functions the algorithms call into — not copy-pasted per algorithm.
- Each algorithm owns its own parameter schema (JSON-serializable via `SceneConfig`) so the GUI can generically build its controls from that schema rather than hardcoding per-algorithm UI where avoidable.

## 3. Separation of concerns

- `artgen_core` (algorithms, color, palette, rendering, I/O) must not depend on SDL2, ImGui, or any UI library.
- `src/gui` depends on `artgen_core`, never the reverse.
- The SDL2 `--preview` window and the ImGui GUI are independent consumers of `artgen_core` — neither is required by the other.

## 4. Config as the source of truth

Scene state lives in `SceneConfig` (JSON), not scattered across UI widget state. The GUI reads/writes `SceneConfig`; it doesn't invent parallel state that the CLI can't reproduce. Anything a user can do in the GUI should be reproducible by hand-editing or generating a scene JSON file.

## 5. Test what isn't visual

Algorithm math (color conversions, escape-time iteration, palette interpolation, etc.) gets Catch2 unit tests in `tests/`. GUI layout and interaction are exempt from automated testing but should be manually verified (see `/verify` workflow) before calling a change done.

## 6. Extend the menu, don't bypass it

As new application modules are added (beyond generative art), each one is a peer entry on the main menu (`AppMode` in `Gui.cpp`), not a special case bolted onto the Generative Art screen. A module should be addable without touching the Controls/Preview panels of unrelated modules.

## 7. No premature abstraction

Don't generalize an interface (`IAlgorithm`, `AppMode`, plugin registries) ahead of a second real use case. Two algorithms justify a shared base; one doesn't. Apply principles 1–6 when the need is concrete, not speculatively.

## 8. Linux support is not optional

artgen targets Windows and Linux from day one — don't write code that only works on one.

- Anything gated behind `#ifdef _WIN32` (e.g. the native file dialogs in `Gui.cpp`) needs a working non-Windows path (e.g. a portable file dialog or a text-entry fallback), not a stub that silently no-ops on Linux.
- Avoid Windows-only APIs (`<windows.h>`, `commdlg.h`, registry access, backslash path separators) in `artgen_core` and CLI code; confine any unavoidable platform-specific code behind a narrow `#ifdef` boundary with both sides implemented.
- Use `std::filesystem` for all path handling, never hardcoded path separators or drive letters.
- CMake configuration must build cleanly with a mainstream Linux toolchain (gcc/clang + Ninja or Make), not just MSVC. Periodically sanity-check this — don't let it silently rot.

## 9. Minimize external dependencies

Every new dependency is a build-time, security, and maintenance cost paid forever. Default to the C++ standard library and what's already vendored (nlohmann/json, GLM, stb, SDL2, ImGui, tinyexr) before reaching for anything new.

- Before adding a library, ask: can the standard library or an existing dependency do this? Is the feature small enough to implement directly (as was done for the custom TIFF writer instead of libtiff)?
- A new dependency must earn its place: justify it in the PR/commit description (what it replaces, why hand-rolling isn't reasonable) and prefer small, header-only, permissively-licensed libraries fetched via CMake `FetchContent` over system-installed packages, to keep the build self-contained and reproducible.
- Don't add a dependency for a one-off convenience function. Don't add a UI/runtime framework to solve a problem a few hundred lines of code already solves.

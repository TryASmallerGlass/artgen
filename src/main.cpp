#include "artgen/config/SceneConfig.h"
#include "artgen/IAlgorithm.h"
#include "artgen/PixelBuffer.h"
#include "artgen/PostProcess.h"
#include "artgen/Renderer.h"
#include "artgen/output/ExrWriter.h"
#include "artgen/output/PngWriter.h"
#include "artgen/output/TiffWriter.h"
#ifdef ARTGEN_WITH_SDL2
#include "artgen/Preview.h"
#endif
#include <chrono>
#include <cstdio>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

// ── Output helper ─────────────────────────────────────────────────────────────

static void save(const artgen::PixelBuffer& buf,
                 const std::string& path, int dpi) {
    auto ext_is = [&](const char* e) {
        auto p = path.rfind('.');
        return p != std::string::npos && path.substr(p) == e;
    };
    if (ext_is(".tiff") || ext_is(".tif"))
        artgen::TiffWriter::write(buf, path, dpi);
    else if (ext_is(".exr"))
        artgen::ExrWriter::write(buf, path);
    else
        artgen::PngWriter::write(buf, path);
    std::printf("  Saved : %s\n", path.c_str());
}

// ── Sweep helper ──────────────────────────────────────────────────────────────

struct SweepParam {
    std::string key;
    double start, end, step;
};

static bool parse_sweep(const std::string& s, SweepParam& out) {
    auto eq = s.find('=');
    if (eq == std::string::npos) return false;
    out.key = s.substr(0, eq);
    double vals[3] = {0, 0, 1};
    int got = std::sscanf(s.c_str() + eq + 1, "%lf:%lf:%lf",
                          &vals[0], &vals[1], &vals[2]);
    if (got < 2) return false;
    out.start = vals[0];
    out.end   = vals[1];
    out.step  = (got >= 3) ? vals[2] : 1.0;
    return true;
}

// Apply a single key=value string override. Returns false for unknown keys.
static bool apply_set(artgen::SceneConfig& cfg,
                      const std::string& key, const std::string& val) {
    // String-valued keys
    if (key == "algorithm")     { cfg.algorithm_name = val; return true; }
    if (key == "output")        { cfg.output_path    = val; return true; }
    if (key == "palette")       { cfg.palette_name   = val; return true; }
    if (key == "coloring_mode") { cfg.coloring_mode  = val; return true; }
    if (key == "rd_preset")     { cfg.rd_preset      = val; return true; }
    if (key == "ls_preset")     { cfg.ls_preset      = val; return true; }
    if (key == "ls_fg_color")   { cfg.ls_fg_color    = val; return true; }
    if (key == "ls_bg_color")   { cfg.ls_bg_color    = val; return true; }
    // Numeric keys
    double d = 0.0;
    if (std::sscanf(val.c_str(), "%lf", &d) != 1) return false;
    if (key == "max_iterations")  { cfg.max_iterations  = static_cast<int>(d);      return true; }
    if (key == "color_cycle")     { cfg.color_cycle     = d;                         return true; }
    if (key == "escape_radius")   { cfg.escape_radius   = d;                         return true; }
    if (key == "julia_cr")        { cfg.julia_cr        = d;                         return true; }
    if (key == "julia_ci")        { cfg.julia_ci        = d;                         return true; }
    if (key == "noise_scale")     { cfg.noise_scale     = static_cast<float>(d);    return true; }
    if (key == "noise_seed")      { cfg.noise_seed      = static_cast<uint32_t>(d); return true; }
    if (key == "rd_feed")         { cfg.rd_feed         = static_cast<float>(d);    return true; }
    if (key == "rd_kill")         { cfg.rd_kill         = static_cast<float>(d);    return true; }
    if (key == "palette_phase")   { cfg.palette_phase   = static_cast<float>(d);    return true; }
    if (key == "aa")              { cfg.aa_samples      = static_cast<int>(d);      return true; }
    if (key == "threads")         { cfg.thread_count    = static_cast<int>(d);      return true; }
    if (key == "dpi")             { cfg.output_dpi      = static_cast<int>(d);      return true; }
    if (key == "bit_depth")       { cfg.bit_depth       = static_cast<int>(d);      return true; }
    if (key == "newton_power")    { cfg.newton_power    = static_cast<int>(d);      return true; }
    if (key == "noise_octaves")   { cfg.noise_octaves   = static_cast<int>(d);      return true; }
    if (key == "rd_steps")        { cfg.rd_steps        = static_cast<int>(d);      return true; }
    if (key == "ls_iterations")   { cfg.ls_iterations   = static_cast<int>(d);      return true; }
    if (key == "ls_angle")        { cfg.ls_angle        = static_cast<float>(d);    return true; }
    return false;
}

static void apply_sweep(artgen::SceneConfig& cfg,
                         const std::string& key, double val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.17g", val);
    if (!apply_set(cfg, key, buf))
        std::fprintf(stderr, "Warning: unknown sweep key '%s'\n", key.c_str());
}

// ── Info commands ────────────────────────────────────────────────────────────

static void list_algorithms() {
    std::printf("Algorithms:\n");
    std::printf("  %-22s  %s\n", "mandelbrot",         "Classic z\xb2 + c escape-time");
    std::printf("  %-22s  %s\n", "julia",              "Configurable seed (julia_cr, julia_ci)");
    std::printf("  %-22s  %s\n", "burning_ship",       "Absolute-value variant of Mandelbrot");
    std::printf("  %-22s  %s\n", "newton",             "Root-finding fractal (z^n - 1)");
    std::printf("  %-22s  %s\n", "noise",              "Simplex FBM (fractal Brownian motion)");
    std::printf("  %-22s  %s\n", "reaction_diffusion", "Gray-Scott model; 6 named presets");
    std::printf("  %-22s  %s\n", "lsystem",            "Turtle-graphics L-System; 5 named presets");
}

static void list_presets() {
    std::printf("Palettes (palette=\"name\"):\n");
    std::printf("  classic_mandelbrot  fire  ice  electric  grayscale\n\n");
    std::printf("Generated palette types (palette_type=\"...\"):\n");
    std::printf("  complementary  triadic  analogous  split_complementary\n\n");
    std::printf("Reaction-diffusion presets (rd_preset=\"...\"):\n");
    std::printf("  coral  mitosis  worms  maze  spots  fingerprint\n\n");
    std::printf("L-System presets (ls_preset=\"...\"):\n");
    std::printf("  plant  dragon  sierpinski  hilbert  tree\n");
}

// ── Render one frame ──────────────────────────────────────────────────────────

static artgen::PixelBuffer render_frame(const artgen::SceneConfig& cfg,
                                         const std::string& output_path,
                                         bool do_save = true) {
    auto algo = cfg.create_algorithm();
    artgen::PixelBuffer buf(cfg.viewport.pixel_width, cfg.viewport.pixel_height,
                            cfg.bit_depth == 16
                            ? artgen::PixelBuffer::Depth::U16
                            : artgen::PixelBuffer::Depth::U8);

    artgen::TileRenderer renderer;
    renderer.thread_count  = cfg.thread_count;
    renderer.tile_size     = cfg.tile_size;
    renderer.aa_samples    = cfg.aa_samples;
    renderer.show_progress = true;

    auto t0 = std::chrono::steady_clock::now();
    renderer.render(*algo, buf, cfg.viewport);
    double elapsed = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - t0).count();
    std::printf("  Render: %.2fs\n", elapsed);

    artgen::PostProcessor::apply(buf, cfg.postprocess);

    if (do_save) save(buf, output_path, cfg.output_dpi);
    return buf;
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char* argv[]) {
    std::string              config_path = "scenes/mandelbrot_default.json";
    std::string              sweep_spec;
    std::string              animate_spec;
    std::vector<std::string> set_specs;
    bool                     do_preview  = false;

    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--list-algorithms") == 0) { list_algorithms(); return 0; }
        if (std::strcmp(argv[i], "--list-presets")    == 0) { list_presets();    return 0; }
        if (std::strcmp(argv[i], "--preview")         == 0) { do_preview = true; continue; }
        if (i + 1 < argc) {
            if (std::strcmp(argv[i], "--config")  == 0) { config_path  = argv[++i]; continue; }
            if (std::strcmp(argv[i], "--sweep")   == 0) { sweep_spec   = argv[++i]; continue; }
            if (std::strcmp(argv[i], "--animate") == 0) { animate_spec = argv[++i]; continue; }
            if (std::strcmp(argv[i], "--set")     == 0) { set_specs.push_back(argv[++i]); continue; }
        }
    }

    try {
        artgen::SceneConfig base = artgen::SceneConfig::from_json(config_path);

        for (const auto& spec : set_specs) {
            auto eq = spec.find('=');
            if (eq == std::string::npos) {
                std::fprintf(stderr, "Warning: --set '%s' ignored (expected key=value)\n", spec.c_str());
                continue;
            }
            if (!apply_set(base, spec.substr(0, eq), spec.substr(eq + 1)))
                std::fprintf(stderr, "Warning: --set unknown key '%s'\n", spec.substr(0, eq).c_str());
        }

        std::printf("Config : %s\n", config_path.c_str());
        std::printf("Size   : %d x %d\n",
                    base.viewport.pixel_width, base.viewport.pixel_height);
        std::printf("Algo   : %s\n", base.algorithm_name.c_str());

        // ── Single render ─────────────────────────────────────────────────────
        if (sweep_spec.empty() && animate_spec.empty()) {
            auto buf = render_frame(base, base.output_path, true);

#ifdef ARTGEN_WITH_SDL2
            if (do_preview) {
                auto algo = base.create_algorithm();
                artgen::run_preview(*algo, base.viewport, buf);
            }
#else
            if (do_preview)
                std::fprintf(stderr,
                    "Warning: --preview not available (built without SDL2)\n");
#endif
            return 0;
        }

        // ── Sweep mode ────────────────────────────────────────────────────────
        if (!sweep_spec.empty()) {
            SweepParam sp;
            if (!parse_sweep(sweep_spec, sp))
                throw std::runtime_error("Bad --sweep format: key=start:end:step");

            std::printf("Sweep  : %s from %.4g to %.4g step %.4g\n",
                        sp.key.c_str(), sp.start, sp.end, sp.step);

            std::string base_path = base.output_path;
            std::string ext  = ".png";
            auto dot = base_path.rfind('.');
            if (dot != std::string::npos) {
                ext       = base_path.substr(dot);
                base_path = base_path.substr(0, dot);
            }

            int frame = 0;
            for (double v = sp.start; v <= sp.end + sp.step * 0.01; v += sp.step) {
                artgen::SceneConfig cfg = base;
                apply_sweep(cfg, sp.key, v);
                char out[1024];
                std::snprintf(out, sizeof(out), "%s_%04d%s",
                              base_path.c_str(), frame, ext.c_str());
                std::printf("\n[Frame %d] %s=%.4g\n", frame, sp.key.c_str(), v);
                render_frame(cfg, out, true);
                ++frame;
            }
            return 0;
        }

        // ── Animate mode (palette phase cycling) ──────────────────────────────
        if (!animate_spec.empty()) {
            SweepParam ap;
            ap.key = "palette_phase";
            // Accept bare "start:end:step" or "palette_phase=start:end:step"
            std::string val_str = animate_spec;
            auto eq = animate_spec.find('=');
            if (eq != std::string::npos) val_str = animate_spec.substr(eq + 1);
            double vals[3] = {0, 1, 0.05};
            int got = std::sscanf(val_str.c_str(), "%lf:%lf:%lf",
                                  &vals[0], &vals[1], &vals[2]);
            if (got < 2)
                throw std::runtime_error("Bad --animate format: start:end:step");
            ap.start = vals[0]; ap.end = vals[1];
            ap.step  = (got >= 3) ? vals[2] : 0.05;

            std::printf("Animate: palette phase %.3f to %.3f in %d frames\n",
                        ap.start, ap.end, static_cast<int>((ap.end - ap.start) / ap.step) + 1);

            std::string base_path = base.output_path;
            std::string ext  = ".png";
            auto dot = base_path.rfind('.');
            if (dot != std::string::npos) {
                ext       = base_path.substr(dot);
                base_path = base_path.substr(0, dot);
            }

            int frame = 0;
            for (double v = ap.start; v <= ap.end + ap.step * 0.01; v += ap.step) {
                artgen::SceneConfig cfg = base;
                cfg.palette_phase = static_cast<float>(v);
                char out[1024];
                std::snprintf(out, sizeof(out), "%s_%04d%s",
                              base_path.c_str(), frame, ext.c_str());
                std::printf("\n[Frame %d] phase=%.3f\n", frame, cfg.palette_phase);
                render_frame(cfg, out, true);
                ++frame;
            }
            return 0;
        }

    } catch (const std::exception& e) {
        std::fprintf(stderr, "Error: %s\n", e.what());
        return 1;
    }
    return 0;
}

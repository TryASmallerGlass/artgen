#include "artgen/config/SceneConfig.h"
#include "artgen/AlgorithmRegistry.h"
#include "artgen/IAlgorithm.h"
#include "artgen/PixelBuffer.h"
#include "artgen/PostProcess.h"
#include "artgen/Renderer.h"
#include "artgen/output/PngWriter.h"
#include "artgen/output/TiffWriter.h"
#include "artgen/output/ExrWriter.h"
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
    auto ends_with = [&](const std::string& ext) {
        return path.size() >= ext.size() &&
               path.compare(path.size() - ext.size(), ext.size(), ext) == 0;
    };
    if (ends_with(".tiff") || ends_with(".tif"))
        artgen::TiffWriter::write(buf, path, dpi);
    else if (ends_with(".exr"))
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

enum class ApplySetResult { Applied, UnknownKey, InvalidValue };

static ApplySetResult apply_set(artgen::SceneConfig& cfg,
                                const std::string& key, const std::string& val) {
    // String-valued keys
    if (key == "algorithm")     { cfg.algorithm_name = val; return ApplySetResult::Applied; }
    if (key == "output")        { cfg.output_path    = val; return ApplySetResult::Applied; }
    if (key == "palette")       { cfg.palette_name   = val; return ApplySetResult::Applied; }
    if (key == "coloring_mode") { cfg.coloring_mode  = val; return ApplySetResult::Applied; }
    if (key == "rd_preset")     { cfg.rd_preset      = val; return ApplySetResult::Applied; }
    if (key == "ls_preset")     { cfg.ls_preset      = val; return ApplySetResult::Applied; }
    if (key == "ls_fg_color")   { cfg.ls_fg_color    = val; return ApplySetResult::Applied; }
    if (key == "ls_bg_color")   { cfg.ls_bg_color    = val; return ApplySetResult::Applied; }
    // Numeric keys
    double d = 0.0;
    auto parse_number = [&]() {
        if (std::sscanf(val.c_str(), "%lf", &d) != 1) return false;
        return true;
    };
    if (key == "max_iterations")  { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.max_iterations  = static_cast<int>(d);      return ApplySetResult::Applied; }
    if (key == "color_cycle")     { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.color_cycle     = d;                         return ApplySetResult::Applied; }
    if (key == "escape_radius")   { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.escape_radius   = d;                         return ApplySetResult::Applied; }
    if (key == "julia_cr")        { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.julia_cr        = d;                         return ApplySetResult::Applied; }
    if (key == "julia_ci")        { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.julia_ci        = d;                         return ApplySetResult::Applied; }
    if (key == "noise_scale")     { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.noise_scale     = static_cast<float>(d);    return ApplySetResult::Applied; }
    if (key == "noise_seed")      { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.noise_seed      = static_cast<uint32_t>(d); return ApplySetResult::Applied; }
    if (key == "rd_feed")         { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.rd_feed         = static_cast<float>(d);    return ApplySetResult::Applied; }
    if (key == "rd_kill")         { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.rd_kill         = static_cast<float>(d);    return ApplySetResult::Applied; }
    if (key == "palette_phase")   { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.palette_phase   = static_cast<float>(d);    return ApplySetResult::Applied; }
    if (key == "aa")              { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.aa_samples      = static_cast<int>(d);      return ApplySetResult::Applied; }
    if (key == "threads")         { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.thread_count    = static_cast<int>(d);      return ApplySetResult::Applied; }
    if (key == "dpi")             { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.output_dpi      = static_cast<int>(d);      return ApplySetResult::Applied; }
    if (key == "bit_depth")       { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.bit_depth       = static_cast<int>(d);      return ApplySetResult::Applied; }
    if (key == "newton_power")    { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.newton_power    = static_cast<int>(d);      return ApplySetResult::Applied; }
    if (key == "noise_octaves")   { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.noise_octaves   = static_cast<int>(d);      return ApplySetResult::Applied; }
    if (key == "rd_steps")        { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.rd_steps        = static_cast<int>(d);      return ApplySetResult::Applied; }
    if (key == "ls_iterations")   { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.ls_iterations   = static_cast<int>(d);      return ApplySetResult::Applied; }
    if (key == "ls_angle")        { if (!parse_number()) return ApplySetResult::InvalidValue; cfg.ls_angle        = static_cast<float>(d);    return ApplySetResult::Applied; }
    return ApplySetResult::UnknownKey;
}

static void apply_sweep(artgen::SceneConfig& cfg,
                         const std::string& key, double val) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.17g", val);
    auto result = apply_set(cfg, key, buf);
    if (result == ApplySetResult::UnknownKey)
        std::fprintf(stderr, "Warning: unknown sweep key '%s'\n", key.c_str());
    else if (result == ApplySetResult::InvalidValue)
        std::fprintf(stderr, "Warning: invalid sweep value '%s' for key '%s'\n", buf, key.c_str());
}

// ── Info commands ────────────────────────────────────────────────────────────

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

// ── Post-render quality check ─────────────────────────────────────────────────
//
// Samples every 16th pixel and computes mean luminance + variance.
// Prints an actionable warning if the output is near-black or completely uniform
// (both indicate a likely parameter misconfiguration rather than an artistic choice).

static void check_render_quality(const artgen::PixelBuffer& buf,
                                  const std::string& algo_name)
{
    const int W = buf.width(), H = buf.height();
    const int stride = 4; // sample every 4th pixel in each dimension

    double sum_lum  = 0.0;
    double sum_lsq  = 0.0;
    int    n        = 0;
    float  max_lum  = 0.0f;

    for (int y = 0; y < H; y += stride) {
        for (int x = 0; x < W; x += stride) {
            artgen::RGBA c = buf.get(x, y);
            // Rec.709 luminance
            float lum = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
            sum_lum += lum;
            sum_lsq += lum * lum;
            if (lum > max_lum) max_lum = lum;
            ++n;
        }
    }

    if (n == 0) return;

    const double mean_lum = sum_lum / n;
    const double variance  = (sum_lsq / n) - (mean_lum * mean_lum);

    // Thresholds (tuned empirically):
    //   mean < 0.02  → almost all pixels are near-black
    //   max  < 0.05  → no bright pixels at all (density histograms with empty fields)
    //   variance < 5e-5 → image is effectively one flat colour
    const bool near_black = (mean_lum < 0.02 && max_lum < 0.05);
    const bool flat_colour = (variance < 5e-5 && mean_lum > 0.02);

    if (!near_black && !flat_colour) return;

    std::fprintf(stderr, "\n");
    std::fprintf(stderr, "  *** Quality warning [%s] ***\n", algo_name.c_str());

    if (near_black) {
        std::fprintf(stderr,
            "  Output is near-black (mean luminance %.4f, max %.4f).\n"
            "  The parameter combination produced no visible structure.\n",
            mean_lum, max_lum);

        // Algorithm-specific hints
        if (algo_name == "ikeda") {
            std::fprintf(stderr,
                "  Hint: attractor_u < 0.75 produces periodic orbits, not a chaotic\n"
                "  attractor. Set attractor_u in [0.75, 0.90] for visible ribbons.\n");
        } else if (algo_name == "lyapunov") {
            std::fprintf(stderr,
                "  Hint: the viewport must stay within (2, 4) x (2, 4) for the\n"
                "  logistic map to remain bounded. Values outside this range cause\n"
                "  divergence. Also check that the sequence contains both 'A' and 'B'.\n");
        } else if (algo_name == "attractor") {
            std::fprintf(stderr,
                "  Hint: the orbit may fall entirely outside the viewport.\n"
                "  Widen the viewport or adjust a/b/c/d so the attractor centre\n"
                "  falls within [real_min, real_max] x [imag_min, imag_max].\n");
        } else if (algo_name == "nova") {
            std::fprintf(stderr,
                "  Hint: try reducing nova_escape_radius or increasing\n"
                "  nova_max_iterations, or widen the viewport to cover the\n"
                "  region where roots are located.\n");
        } else {
            std::fprintf(stderr,
                "  Hint: review parameter ranges — consult docs/algorithms.md for\n"
                "  valid input bounds for '%s'.\n", algo_name.c_str());
        }
    }

    if (flat_colour) {
        std::fprintf(stderr,
            "  Output is a flat uniform colour (variance %.2e, mean %.4f).\n"
            "  The algorithm produced no spatial variation — check parameter ranges.\n",
            variance, mean_lum);
    }

    std::fprintf(stderr, "\n");
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

    check_render_quality(buf, cfg.algorithm_name);

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
        if (std::strcmp(argv[i], "--list-algorithms") == 0) {
            std::printf("Registered algorithms:\n");
            for (const auto& name : artgen::AlgorithmRegistry::names())
                std::printf("  %s\n", name.c_str());
            return 0;
        }
        if (std::strcmp(argv[i], "--list-presets") == 0) { list_presets(); return 0; }
        if (std::strcmp(argv[i], "--preview")      == 0) { do_preview = true; continue; }
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
            auto key = spec.substr(0, eq);
            auto val = spec.substr(eq + 1);
            auto result = apply_set(base, key, val);
            if (result == ApplySetResult::UnknownKey)
                std::fprintf(stderr, "Warning: --set unknown key '%s'\n", key.c_str());
            else if (result == ApplySetResult::InvalidValue)
                std::fprintf(stderr, "Warning: --set invalid value '%s' for key '%s'\n", val.c_str(), key.c_str());
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

#include "artgen/config/SceneConfig.h"
#include "artgen/IAlgorithm.h"
#include "artgen/PixelBuffer.h"
#include "artgen/PostProcess.h"
#include "artgen/Renderer.h"
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
    bool tiff = path.size() >= 5 &&
                (path.substr(path.size()-5) == ".tiff" ||
                 path.substr(path.size()-4) == ".tif");
    if (tiff)
        artgen::TiffWriter::write(buf, path, dpi);
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

static void apply_sweep(artgen::SceneConfig& cfg,
                         const std::string& key, double val) {
    if      (key == "max_iterations")  cfg.max_iterations  = static_cast<int>(val);
    else if (key == "color_cycle")     cfg.color_cycle     = val;
    else if (key == "escape_radius")   cfg.escape_radius   = val;
    else if (key == "julia_cr")        cfg.julia_cr        = val;
    else if (key == "julia_ci")        cfg.julia_ci        = val;
    else if (key == "noise_scale")     cfg.noise_scale     = static_cast<float>(val);
    else if (key == "noise_seed")      cfg.noise_seed      = static_cast<uint32_t>(val);
    else if (key == "rd_feed")         cfg.rd_feed         = static_cast<float>(val);
    else if (key == "rd_kill")         cfg.rd_kill         = static_cast<float>(val);
    else if (key == "palette_phase")   cfg.palette_phase   = static_cast<float>(val);
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
    std::string config_path = "scenes/mandelbrot_default.json";
    std::string sweep_spec;
    std::string animate_spec;
    bool        do_preview  = false;

    for (int i = 1; i < argc; ++i) {
        if (i + 1 < argc) {
            if (std::strcmp(argv[i], "--config")  == 0) config_path  = argv[++i];
            if (std::strcmp(argv[i], "--sweep")   == 0) sweep_spec   = argv[++i];
            if (std::strcmp(argv[i], "--animate") == 0) animate_spec = argv[++i];
        }
        if (std::strcmp(argv[i], "--preview") == 0) do_preview = true;
    }

    try {
        artgen::SceneConfig base = artgen::SceneConfig::from_json(config_path);

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

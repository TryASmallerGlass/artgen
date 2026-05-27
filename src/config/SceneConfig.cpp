#include "artgen/config/SceneConfig.h"
#include "artgen/IAlgorithm.h"
#include "artgen/algorithms/Mandelbrot.h"
#include "artgen/algorithms/Julia.h"
#include "artgen/algorithms/BurningShip.h"
#include "artgen/algorithms/Newton.h"
#include "artgen/algorithms/NoiseField.h"
#include "artgen/algorithms/ReactionDiffusion.h"
#include "artgen/algorithms/LSystem.h"
#include "artgen/PaletteGenerator.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <cstdio>
#include <sstream>

namespace artgen {

// ── JSON helpers ──────────────────────────────────────────────────────────────

static RGBA parse_hex(const std::string& hex) {
    int r = 0, g = 0, b = 0, a = 255;
    if (hex.size() >= 7) {
        // MSVC sscanf_s requires explicit buffer sizes for %s/%c; for %x no size needed
        #ifdef _MSC_VER
        sscanf_s(hex.c_str() + 1, "%2x%2x%2x", &r, &g, &b);
        if (hex.size() >= 9) sscanf_s(hex.c_str() + 7, "%2x", &a);
        #else
        std::sscanf(hex.c_str() + 1, "%2x%2x%2x", &r, &g, &b);
        if (hex.size() >= 9) std::sscanf(hex.c_str() + 7, "%2x", &a);
        #endif
    }
    return {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
}

static Palette resolve_named(const std::string& name) {
    if (name == "fire")     return Palette::fire();
    if (name == "ice")      return Palette::ice();
    if (name == "electric") return Palette::electric();
    if (name == "grayscale") return Palette::grayscale();
    return Palette::classic_mandelbrot();
}

// ── Config loading ─────────────────────────────────────────────────────────────

SceneConfig SceneConfig::from_json(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open())
        throw std::runtime_error("Cannot open config: " + path);

    nlohmann::json j = nlohmann::json::parse(f);
    SceneConfig cfg;

    if (j.contains("viewport")) {
        auto& vp = j["viewport"];
        if (vp.contains("real_min"))  cfg.viewport.real_min     = vp["real_min"];
        if (vp.contains("real_max"))  cfg.viewport.real_max     = vp["real_max"];
        if (vp.contains("imag_min"))  cfg.viewport.imag_min     = vp["imag_min"];
        if (vp.contains("imag_max"))  cfg.viewport.imag_max     = vp["imag_max"];
        if (vp.contains("width"))     cfg.viewport.pixel_width  = vp["width"];
        if (vp.contains("height"))    cfg.viewport.pixel_height = vp["height"];
    }

    if (j.contains("algorithm"))         cfg.algorithm_name    = j["algorithm"];
    if (j.contains("output"))            cfg.output_path       = j["output"];
    if (j.contains("bit_depth"))         cfg.bit_depth         = j["bit_depth"];
    if (j.contains("max_iterations"))    cfg.max_iterations    = j["max_iterations"];
    if (j.contains("escape_radius"))     cfg.escape_radius     = j["escape_radius"];
    if (j.contains("smooth_coloring"))   cfg.smooth_coloring   = j["smooth_coloring"];
    if (j.contains("color_cycle"))       cfg.color_cycle       = j["color_cycle"];
    if (j.contains("coloring_mode"))     cfg.coloring_mode     = j["coloring_mode"];

    if (j.contains("palette"))           cfg.palette_name      = j["palette"];
    if (j.contains("palette_lab"))       cfg.palette_lab       = j["palette_lab"];
    if (j.contains("palette_type"))      cfg.palette_type      = j["palette_type"];
    if (j.contains("palette_hsl")) {
        auto& hsl = j["palette_hsl"];
        cfg.palette_hsl_h = hsl[0];
        cfg.palette_hsl_s = hsl[1];
        cfg.palette_hsl_l = hsl[2];
    }

    if (j.contains("palette_stops")) {
        cfg.palette_type = "custom";
        for (const auto& s : j["palette_stops"])
            cfg.palette_custom_stops.push_back({s["pos"], std::string(s["color"])});
    }

    if (j.contains("julia_cr"))          cfg.julia_cr          = j["julia_cr"];
    if (j.contains("julia_ci"))          cfg.julia_ci          = j["julia_ci"];

    if (j.contains("newton_power"))      cfg.newton_power      = j["newton_power"];
    if (j.contains("newton_tolerance"))  cfg.newton_tolerance  = j["newton_tolerance"];
    if (j.contains("newton_saturation")) cfg.newton_saturation = j["newton_saturation"];

    if (j.contains("noise_octaves"))     cfg.noise_octaves     = j["noise_octaves"];
    if (j.contains("noise_persistence")) cfg.noise_persistence = j["noise_persistence"];
    if (j.contains("noise_lacunarity"))  cfg.noise_lacunarity  = j["noise_lacunarity"];
    if (j.contains("noise_scale"))       cfg.noise_scale       = j["noise_scale"];
    if (j.contains("noise_seed"))        cfg.noise_seed        = static_cast<uint32_t>(int(j["noise_seed"]));

    if (j.contains("rd_Du"))      cfg.rd_Du     = j["rd_Du"];
    if (j.contains("rd_Dv"))      cfg.rd_Dv     = j["rd_Dv"];
    if (j.contains("rd_feed"))    cfg.rd_feed   = j["rd_feed"];
    if (j.contains("rd_kill"))    cfg.rd_kill   = j["rd_kill"];
    if (j.contains("rd_dt"))      cfg.rd_dt     = j["rd_dt"];
    if (j.contains("rd_steps"))   cfg.rd_steps  = j["rd_steps"];
    if (j.contains("rd_seed"))    cfg.rd_seed   = static_cast<uint32_t>(int(j["rd_seed"]));
    if (j.contains("rd_preset"))  cfg.rd_preset = j["rd_preset"];

    // L-System
    if (j.contains("ls_preset"))     cfg.ls_preset    = j["ls_preset"];
    if (j.contains("ls_axiom"))      cfg.ls_axiom     = j["ls_axiom"];
    if (j.contains("ls_rules"))      cfg.ls_rules     = j["ls_rules"];
    if (j.contains("ls_iterations")) cfg.ls_iterations = j["ls_iterations"];
    if (j.contains("ls_angle"))      cfg.ls_angle     = j["ls_angle"];
    if (j.contains("ls_fg_color"))   cfg.ls_fg_color  = j["ls_fg_color"];
    if (j.contains("ls_bg_color"))   cfg.ls_bg_color  = j["ls_bg_color"];

    // Post-processing
    if (j.contains("postprocess")) {
        auto& pp = j["postprocess"];
        if (pp.contains("gamma"))      cfg.postprocess.gamma      = pp["gamma"];
        if (pp.contains("contrast"))   cfg.postprocess.contrast   = pp["contrast"];
        if (pp.contains("brightness")) cfg.postprocess.brightness = pp["brightness"];
        if (pp.contains("saturation")) cfg.postprocess.saturation = pp["saturation"];
        if (pp.contains("vignette"))   cfg.postprocess.vignette   = pp["vignette"];
    }

    // Palette phase
    if (j.contains("palette_phase")) cfg.palette_phase = j["palette_phase"];

    if (j.contains("threads"))    cfg.thread_count = j["threads"];
    if (j.contains("tile_size"))  cfg.tile_size    = j["tile_size"];
    if (j.contains("aa"))         cfg.aa_samples   = j["aa"];
    if (j.contains("dpi"))        cfg.output_dpi   = j["dpi"];

    return cfg;
}

// ── Palette factory ───────────────────────────────────────────────────────────

static Palette build_palette(const SceneConfig& cfg) {
    auto set_common = [&](Palette& p) {
        p.use_lab_interpolation = cfg.palette_lab;
        p.phase = cfg.palette_phase;
    };
    if (cfg.palette_type == "complementary") {
        auto p = PaletteGenerator::complementary({cfg.palette_hsl_h, cfg.palette_hsl_s, cfg.palette_hsl_l});
        set_common(p); return p;
    }
    if (cfg.palette_type == "triadic") {
        auto p = PaletteGenerator::triadic({cfg.palette_hsl_h, cfg.palette_hsl_s, cfg.palette_hsl_l});
        set_common(p); return p;
    }
    if (cfg.palette_type == "analogous") {
        auto p = PaletteGenerator::analogous({cfg.palette_hsl_h, cfg.palette_hsl_s, cfg.palette_hsl_l});
        set_common(p); return p;
    }
    if (cfg.palette_type == "split_complementary") {
        auto p = PaletteGenerator::split_complementary({cfg.palette_hsl_h, cfg.palette_hsl_s, cfg.palette_hsl_l});
        set_common(p); return p;
    }
    if (cfg.palette_type == "custom" && !cfg.palette_custom_stops.empty()) {
        Palette p;
        set_common(p);
        for (const auto& [pos, hex] : cfg.palette_custom_stops)
            p.add_stop(pos, parse_hex(hex));
        return p;
    }
    // "named" or fallback
    Palette p = resolve_named(cfg.palette_name);
    p.use_lab_interpolation = cfg.palette_lab;
    p.phase = cfg.palette_phase;
    return p;
}

static ColoringMode parse_coloring_mode(const std::string& s) {
    if (s == "histogram_eq") return ColoringMode::HistogramEq;
    if (s == "orbit_trap")   return ColoringMode::OrbitTrap;
    return ColoringMode::Smooth;
}

// ── Algorithm factory ─────────────────────────────────────────────────────────

std::unique_ptr<IAlgorithm> SceneConfig::create_algorithm() const {
    Palette pal = build_palette(*this); // NOLINT(*)

    if (algorithm_name == "mandelbrot" || algorithm_name.empty()) {
        auto a = std::make_unique<MandelbrotAlgorithm>();
        a->max_iterations  = max_iterations;
        a->escape_radius   = escape_radius;
        a->smooth_coloring = smooth_coloring;
        a->color_cycle     = color_cycle;
        a->coloring_mode   = parse_coloring_mode(coloring_mode);
        a->palette         = pal;
        return a;
    }

    if (algorithm_name == "julia") {
        auto a = std::make_unique<JuliaAlgorithm>();
        a->max_iterations  = max_iterations;
        a->escape_radius   = escape_radius;
        a->smooth_coloring = smooth_coloring;
        a->color_cycle     = color_cycle;
        a->coloring_mode   = parse_coloring_mode(coloring_mode);
        a->palette         = pal;
        a->seed_r          = julia_cr;
        a->seed_i          = julia_ci;
        return a;
    }

    if (algorithm_name == "burning_ship") {
        auto a = std::make_unique<BurningShipAlgorithm>();
        a->max_iterations  = max_iterations;
        a->escape_radius   = escape_radius;
        a->smooth_coloring = smooth_coloring;
        a->color_cycle     = color_cycle;
        a->coloring_mode   = parse_coloring_mode(coloring_mode);
        a->palette         = pal;
        return a;
    }

    if (algorithm_name == "newton") {
        auto a = std::make_unique<NewtonAlgorithm>();
        a->power          = newton_power;
        a->max_iterations = max_iterations;
        a->tolerance      = newton_tolerance;
        a->saturation     = newton_saturation;
        return a;
    }

    if (algorithm_name == "noise") {
        auto a = std::make_unique<NoiseFieldAlgorithm>();
        a->octaves     = noise_octaves;
        a->persistence = noise_persistence;
        a->lacunarity  = noise_lacunarity;
        a->scale       = noise_scale;
        a->seed        = noise_seed;
        a->palette     = pal;
        return a;
    }

    if (algorithm_name == "reaction_diffusion" || algorithm_name == "rd") {
        auto a = std::make_unique<ReactionDiffusion>();
        if (!rd_preset.empty()) a->set_preset(rd_preset);
        a->Du    = rd_Du;
        a->Dv    = rd_Dv;
        a->feed  = rd_feed;
        a->kill  = rd_kill;
        a->dt    = rd_dt;
        a->steps = rd_steps;
        a->seed  = rd_seed;
        a->palette = pal;
        return a;
    }

    if (algorithm_name == "lsystem") {
        auto a = std::make_unique<LSystemAlgorithm>();
        if (!ls_preset.empty()) {
            a->set_preset(ls_preset);
        }
        // Override preset with explicit params if provided
        if (!ls_axiom.empty()) a->axiom = ls_axiom;
        if (!ls_rules.empty()) {
            // parse semicolon-separated "X=successor;Y=..."
            std::istringstream ss(ls_rules);
            std::string token;
            while (std::getline(ss, token, ';')) {
                auto eq = token.find('=');
                if (eq != std::string::npos && eq > 0) {
                    char pred = token[0];
                    a->rules.push_back({pred, token.substr(eq + 1)});
                }
            }
        }
        if (ls_iterations > 0) a->iterations = ls_iterations;
        if (ls_angle > 0.0f)   a->angle_deg  = ls_angle;
        if (!ls_fg_color.empty()) a->fg_color = parse_hex(ls_fg_color);
        if (!ls_bg_color.empty()) a->bg_color = parse_hex(ls_bg_color);
        return a;
    }

    throw std::runtime_error("Unknown algorithm: '" + algorithm_name + "'");
}

} // namespace artgen

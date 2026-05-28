#include "artgen/AlgorithmRegistry.h"
#include "artgen/config/SceneConfig.h"
#include <sstream>
#include "artgen/algorithms/Mandelbrot.h"
#include "artgen/algorithms/Julia.h"
#include "artgen/algorithms/BurningShip.h"
#include "artgen/algorithms/Newton.h"
#include "artgen/algorithms/NoiseField.h"
#include "artgen/algorithms/ReactionDiffusion.h"
#include "artgen/algorithms/LSystem.h"
#include "artgen/algorithms/EscapeTime.h"
#include <map>
#include <mutex>

namespace artgen {

// ── Internal storage ──────────────────────────────────────────────────────────

static std::map<std::string, AlgorithmRegistry::Factory>& table() {
    static std::map<std::string, AlgorithmRegistry::Factory> t;
    return t;
}

static std::once_flag s_flag;

// ── Helpers ───────────────────────────────────────────────────────────────────

static ColoringMode parse_mode(const std::string& s) {
    if (s == "histogram_eq") return ColoringMode::HistogramEq;
    if (s == "orbit_trap")   return ColoringMode::OrbitTrap;
    return ColoringMode::Smooth;
}

template<typename T>
static void apply_escape(T& a, const SceneConfig& cfg, const Palette& pal) {
    a.max_iterations  = cfg.max_iterations;
    a.escape_radius   = cfg.escape_radius;
    a.smooth_coloring = cfg.smooth_coloring;
    a.color_cycle     = cfg.color_cycle;
    a.coloring_mode   = parse_mode(cfg.coloring_mode);
    a.palette         = pal;
}

// ── Built-in registration ──────────────────────────────────────────────────────

static void register_builtins() {
    auto& r = AlgorithmRegistry::register_algo;

    r("mandelbrot", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<MandelbrotAlgorithm>();
        apply_escape(*a, cfg, cfg.build_palette());
        return a;
    });

    // "mandelbrot" is the default; accept empty name too
    r("", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<MandelbrotAlgorithm>();
        apply_escape(*a, cfg, cfg.build_palette());
        return a;
    });

    r("julia", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<JuliaAlgorithm>();
        apply_escape(*a, cfg, cfg.build_palette());
        a->seed_r = cfg.julia_cr;
        a->seed_i = cfg.julia_ci;
        return a;
    });

    r("burning_ship", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<BurningShipAlgorithm>();
        apply_escape(*a, cfg, cfg.build_palette());
        return a;
    });

    r("newton", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<NewtonAlgorithm>();
        a->power          = cfg.newton_power;
        a->max_iterations = cfg.max_iterations;
        a->tolerance      = cfg.newton_tolerance;
        a->saturation     = cfg.newton_saturation;
        return a;
    });

    r("noise", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<NoiseFieldAlgorithm>();
        a->octaves     = cfg.noise_octaves;
        a->persistence = cfg.noise_persistence;
        a->lacunarity  = cfg.noise_lacunarity;
        a->scale       = cfg.noise_scale;
        a->seed        = cfg.noise_seed;
        a->palette     = cfg.build_palette();
        return a;
    });

    r("reaction_diffusion", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<ReactionDiffusion>();
        if (!cfg.rd_preset.empty()) a->set_preset(cfg.rd_preset);
        a->Du    = cfg.rd_Du;
        a->Dv    = cfg.rd_Dv;
        a->feed  = cfg.rd_feed;
        a->kill  = cfg.rd_kill;
        a->dt    = cfg.rd_dt;
        a->steps = cfg.rd_steps;
        a->seed  = cfg.rd_seed;
        a->palette = cfg.build_palette();
        return a;
    });

    // Accept "rd" as shorthand
    r("rd", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        return AlgorithmRegistry::create("reaction_diffusion", cfg);
    });

    r("lsystem", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<LSystemAlgorithm>();
        if (!cfg.ls_preset.empty()) a->set_preset(cfg.ls_preset);
        if (!cfg.ls_axiom.empty()) a->axiom = cfg.ls_axiom;
        if (!cfg.ls_rules.empty()) {
            std::istringstream ss(cfg.ls_rules);
            std::string token;
            while (std::getline(ss, token, ';')) {
                auto eq = token.find('=');
                if (eq != std::string::npos && eq > 0)
                    a->rules.push_back({token[0], token.substr(eq + 1)});
            }
        }
        if (cfg.ls_iterations > 0) a->iterations = cfg.ls_iterations;
        if (cfg.ls_angle > 0.0f)   a->angle_deg  = cfg.ls_angle;
        // parse_hex is internal to SceneConfig; call build_palette for fg/bg colour
        // fg/bg are plain RGBA stored in SceneConfig
        return a;
    });
}

// ── Public API ────────────────────────────────────────────────────────────────

void AlgorithmRegistry::ensure_registered() {
    std::call_once(s_flag, register_builtins);
}

void AlgorithmRegistry::register_algo(const std::string& name, Factory f) {
    table()[name] = std::move(f);
}

bool AlgorithmRegistry::has(const std::string& name) {
    ensure_registered();
    return table().count(name) > 0;
}

std::vector<std::string> AlgorithmRegistry::names() {
    ensure_registered();
    std::vector<std::string> v;
    v.reserve(table().size());
    for (const auto& [k, _] : table())
        if (!k.empty()) v.push_back(k); // skip the "" fallback alias
    return v;
}

std::unique_ptr<IAlgorithm> AlgorithmRegistry::create(
    const std::string& name, const SceneConfig& cfg)
{
    ensure_registered();
    auto it = table().find(name);
    if (it == table().end()) return nullptr;
    return it->second(cfg);
}

} // namespace artgen

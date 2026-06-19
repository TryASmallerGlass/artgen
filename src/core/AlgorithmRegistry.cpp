#include "artgen/AlgorithmRegistry.h"
#include "artgen/config/SceneConfig.h"
#include <cstdio>
#include <sstream>
#include "artgen/algorithms/Mandelbrot.h"
#include "artgen/algorithms/Julia.h"
#include "artgen/algorithms/BurningShip.h"
#include "artgen/algorithms/Newton.h"
#include "artgen/algorithms/NoiseField.h"
#include "artgen/algorithms/ReactionDiffusion.h"
#include "artgen/algorithms/LSystem.h"
#include "artgen/algorithms/EscapeTime.h"
#include "artgen/algorithms/Multibrot.h"
#include "artgen/algorithms/Tricorn.h"
#include "artgen/algorithms/Attractor.h"
#include "artgen/algorithms/Plasma.h"
#include "artgen/algorithms/Voronoi.h"
#include "artgen/algorithms/Lyapunov.h"
#include "artgen/algorithms/Nova.h"
#include "artgen/algorithms/CyclicCA.h"
#include "artgen/algorithms/Physarum.h"
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
    if (s.empty() || s == "smooth") return ColoringMode::Smooth;
    if (s == "histogram_eq")        return ColoringMode::HistogramEq;
    if (s == "orbit_trap")          return ColoringMode::OrbitTrap;
    std::fprintf(stderr,
        "Warning: unrecognized coloring_mode \"%s\" — falling back to \"smooth\". "
        "Valid values: \"smooth\", \"histogram_eq\", \"orbit_trap\".\n",
        s.c_str());
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

    r("multibrot", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<MultibrotAlgorithm>();
        apply_escape(*a, cfg, cfg.build_palette());
        a->power = cfg.multibrot_power;   // also updates smooth-coloring log base
        return a;
    });

    r("tricorn", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<TricornAlgorithm>();
        apply_escape(*a, cfg, cfg.build_palette());
        return a;
    });

    r("attractor", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<AttractorAlgorithm>();
        if (cfg.attractor_type == "dejong") a->type = AttractorType::DeJong;
        a->a          = cfg.attractor_a;
        a->b          = cfg.attractor_b;
        a->c          = cfg.attractor_c;
        a->d          = cfg.attractor_d;
        a->iterations = cfg.attractor_iterations;
        a->palette    = cfg.build_palette();
        // Warn if the default viewport is likely to clip the orbit entirely.
        // Clifford and De Jong orbits are bounded by the trig functions (roughly
        // ±3 in each direction) so a very tight viewport can hide the attractor.
        const double rw = cfg.viewport.real_max - cfg.viewport.real_min;
        const double iw = cfg.viewport.imag_max - cfg.viewport.imag_min;
        if (rw < 0.5 || iw < 0.5)
            std::fprintf(stderr,
                "Warning [attractor]: viewport is very small (%.2f x %.2f). "
                "The orbit may fall outside it entirely. "
                "Recommended: real/imag range of at least 6 units centred on 0.\n",
                rw, iw);
        return a;
    });

    r("plasma", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<PlasmaAlgorithm>();
        a->roughness = cfg.plasma_roughness;
        a->seed      = cfg.plasma_seed;
        a->octaves   = cfg.plasma_octaves;
        a->palette   = cfg.build_palette();
        return a;
    });

    r("voronoi", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<VoronoiAlgorithm>();
        a->num_seeds = cfg.voronoi_num_seeds;
        a->seed      = cfg.voronoi_seed;
        if      (cfg.voronoi_mode == "distance") a->mode = VoronoiMode::Distance;
        else if (cfg.voronoi_mode == "edge")     a->mode = VoronoiMode::Edge;
        if      (cfg.voronoi_metric == "manhattan")  a->metric = VoronoiMetric::Manhattan;
        else if (cfg.voronoi_metric == "chebyshev")  a->metric = VoronoiMetric::Chebyshev;
        a->palette = cfg.build_palette();
        return a;
    });

    // ── Ikeda map attractor ───────────────────────────────────────────────────

    r("ikeda", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        // Parameter boundary check: u controls nonlinearity.
        //   u < 0.7  → the map settles onto a periodic orbit, not a strange
        //               attractor.  The density histogram will be near-black.
        //   u > 0.98 → valid but the attractor becomes very diffuse.
        if (cfg.attractor_u < 0.75)
            std::fprintf(stderr,
                "Warning [ikeda]: attractor_u=%.3f is below 0.75. "
                "The Ikeda map will produce a periodic orbit rather than a "
                "chaotic strange attractor — the output will be near-black. "
                "Recommended range: 0.75 – 0.90.\n",
                cfg.attractor_u);
        else if (cfg.attractor_u > 0.90)
            std::fprintf(stderr,
                "Warning [ikeda]: attractor_u=%.3f is above 0.90. "
                "The Ikeda map enters periodic orbit windows above this value — "
                "the output is likely to be near-black or very sparse. "
                "Recommended range: 0.75 – 0.90.\n",
                cfg.attractor_u);

        auto a   = std::make_unique<AttractorAlgorithm>();
        a->type  = AttractorType::Ikeda;
        a->u     = cfg.attractor_u;
        a->iterations = cfg.attractor_iterations;
        a->warmup     = 1000;
        a->palette    = cfg.build_palette();
        return a;
    });

    // ── Lyapunov fractal ──────────────────────────────────────────────────────

    r("lyapunov", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        // The logistic map r*x*(1-x) only remains bounded for r in (0, 4].
        // Viewport axes map to the 'a' (real) and 'b' (imag) parameters.
        // Values outside this range cause divergence → near-black output.
        const double rmin = cfg.viewport.real_min, rmax = cfg.viewport.real_max;
        const double imin = cfg.viewport.imag_min, imax = cfg.viewport.imag_max;
        if (rmin < 0.0 || rmax > 4.0 || imin < 0.0 || imax > 4.0)
            std::fprintf(stderr,
                "Warning [lyapunov]: viewport (%.2f–%.2f, %.2f–%.2f) extends "
                "outside the valid logistic-map range (0, 4]. "
                "Pixels in the out-of-range region will be black. "
                "Recommended viewport: real and imag both within [2.0, 4.0].\n",
                rmin, rmax, imin, imax);

        // Sequence must contain at least one 'A' and one 'B'; a sequence of only
        // one type degenerates to a fixed-r logistic map (spatially constant output).
        const auto& seq = cfg.lyapunov_sequence;
        const bool has_a = seq.find('A') != std::string::npos;
        const bool has_b = seq.find('B') != std::string::npos;
        if (!has_a || !has_b)
            std::fprintf(stderr,
                "Warning [lyapunov]: sequence \"%s\" does not contain both 'A' "
                "and 'B'. The output will be spatially constant (flat colour). "
                "Use a sequence such as \"AB\" or \"AABAB\".\n",
                seq.c_str());

        auto a = std::make_unique<LyapunovAlgorithm>();
        a->sequence   = cfg.lyapunov_sequence;
        a->warmup     = cfg.lyapunov_warmup;
        a->iterations = cfg.lyapunov_iterations;
        a->seed_x     = cfg.lyapunov_seed_x;
        a->palette    = cfg.build_palette();
        return a;
    });

    // ── Nova fractal ──────────────────────────────────────────────────────────

    r("nova", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        if (cfg.nova_power < 2)
            std::fprintf(stderr,
                "Warning [nova]: nova_power=%d is below 2. "
                "The polynomial z^p - 1 has no useful root structure for p < 2. "
                "Recommended: nova_power >= 2.\n",
                cfg.nova_power);

        // Julia mode with low iteration caps tends to produce near-black output
        // because orbits need many steps to settle into a basin or escape.
        if (cfg.nova_type == "julia" && cfg.nova_max_iterations < 128)
            std::fprintf(stderr,
                "Warning [nova]: nova_type=\"julia\" with nova_max_iterations=%d "
                "is likely too low — most pixels will exhaust the iteration cap "
                "without converging or escaping, producing a dark image. "
                "Recommended: nova_max_iterations >= 256 for Julia mode.\n",
                cfg.nova_max_iterations);

        auto a = std::make_unique<NovaAlgorithm>();
        a->power          = cfg.nova_power;
        a->relaxation     = cfg.nova_relaxation;
        a->julia_mode     = (cfg.nova_type == "julia");
        a->seed_r         = cfg.nova_seed_r;
        a->seed_i         = cfg.nova_seed_i;
        a->max_iterations = cfg.nova_max_iterations;
        a->tolerance      = cfg.nova_tolerance;
        a->escape_radius  = cfg.nova_escape_radius;
        a->saturation     = cfg.nova_saturation;
        a->palette        = cfg.build_palette();
        return a;
    });

    // ── Cyclic Cellular Automaton ─────────────────────────────────────────────

    r("cyclic_ca", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<CyclicCAAlgorithm>();
        a->states  = cfg.cca_states;
        a->moore   = (cfg.cca_neighborhood != "vonneumann");
        a->steps   = cfg.cca_steps;
        a->seed    = cfg.cca_seed;
        a->palette = cfg.build_palette();
        return a;
    });

    // ── Physarum Polycephalum ─────────────────────────────────────────────────

    r("physarum", [](const SceneConfig& cfg) -> std::unique_ptr<IAlgorithm> {
        auto a = std::make_unique<PhysarumAlgorithm>();
        a->num_agents     = cfg.physarum_num_agents;
        a->steps          = cfg.physarum_steps;
        a->sensor_angle   = cfg.physarum_sensor_angle;
        a->sensor_dist    = cfg.physarum_sensor_dist;
        a->rotation_angle = cfg.physarum_rotation_angle;
        a->step_size      = cfg.physarum_step_size;
        a->deposit        = cfg.physarum_deposit;
        a->decay          = cfg.physarum_decay;
        a->seed           = cfg.physarum_seed;
        a->palette        = cfg.build_palette();
        return a;
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

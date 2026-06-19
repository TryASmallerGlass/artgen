#pragma once
#include "artgen/Viewport.h"
#include "artgen/Palette.h"
#include "artgen/PostProcess.h"
#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace artgen {

class IAlgorithm;

struct SceneConfig {
    Viewport    viewport;
    std::string algorithm_name   = "mandelbrot";
    std::string output_path      = "output.png";
    int         bit_depth        = 8;

    // Escape-time params (Mandelbrot, Julia, BurningShip)
    int         max_iterations   = 1000;
    double      escape_radius    = 2.0;
    bool        smooth_coloring  = true;
    double      color_cycle      = 64.0;
    std::string coloring_mode    = "smooth"; // "smooth" | "histogram_eq" | "orbit_trap"

    // Palette selection
    std::string palette_name     = "classic_mandelbrot";
    bool        palette_lab      = false;   // use LAB interpolation
    float       palette_phase    = 0.0f;   // cyclic phase offset [0,1)
    // palette_type: "named" | "complementary" | "triadic" | "analogous" | "split_complementary" | "custom"
    std::string palette_type     = "named";
    float       palette_hsl_h    = 200.0f;  // base HSL for generated palettes
    float       palette_hsl_s    = 0.8f;
    float       palette_hsl_l    = 0.5f;
    // Custom palette stops: list of (position, "#RRGGBB[AA]") pairs
    std::vector<std::pair<float, std::string>> palette_custom_stops;

    // Julia seed
    double julia_cr =  -0.7;
    double julia_ci =   0.27015;

    // Newton
    int    newton_power     = 3;
    double newton_tolerance = 1e-6;
    float  newton_saturation = 0.8f;

    // Noise field
    int      noise_octaves     = 6;
    float    noise_persistence = 0.5f;
    float    noise_lacunarity  = 2.0f;
    float    noise_scale       = 1.0f;
    uint32_t noise_seed        = 42;

    // Reaction-diffusion
    float    rd_Du     = 0.2097f;
    float    rd_Dv     = 0.1050f;
    float    rd_feed   = 0.0545f;
    float    rd_kill   = 0.062f;
    float    rd_dt     = 1.0f;
    int      rd_steps  = 8000;
    uint32_t rd_seed   = 42;
    std::string rd_preset; // "coral" "mitosis" "worms" "maze" "spots" "fingerprint"

    // Multibrot
    double multibrot_power = 3.0;

    // Strange attractor
    // "ikeda" is a separate top-level algorithm ("algorithm": "ikeda"), not a value
    // of attractor_type — this field only ever selects between Clifford and De Jong.
    std::string attractor_type       = "clifford"; // "clifford" | "dejong"
    double      attractor_a          = -1.4;
    double      attractor_b          =  1.6;
    double      attractor_c          =  1.0;
    double      attractor_d          =  0.7;
    double      attractor_u          =  0.9;   // Ikeda: tuning parameter
    int         attractor_iterations = 8'000'000;

    // Plasma / Diamond-Square
    float    plasma_roughness = 0.5f;
    uint32_t plasma_seed      = 42;
    int      plasma_octaves   = 8;

    // Voronoi
    int         voronoi_num_seeds = 32;
    uint32_t    voronoi_seed      = 42;
    std::string voronoi_mode      = "cells";    // "cells" | "distance" | "edge"
    std::string voronoi_metric    = "euclidean"; // "euclidean" | "manhattan" | "chebyshev"

    // L-System
    std::string ls_preset;     // "plant" "dragon" "sierpinski" "hilbert" "tree"
    std::string ls_axiom;
    std::string ls_rules;      // semicolon-separated "X=successor;Y=successor"
    int         ls_iterations  = 5;
    float       ls_angle       = 25.7f;
    std::string ls_fg_color    = "#33CC55";
    std::string ls_bg_color    = "#080D08";

    // Lyapunov fractal
    std::string lyapunov_sequence   = "AB";  // 'A' = real coord, 'B' = imag coord
    int         lyapunov_warmup     = 200;
    int         lyapunov_iterations = 1000;
    double      lyapunov_seed_x     = 0.5;

    // Nova fractal
    int         nova_power          = 3;
    double      nova_relaxation     = 1.0;
    std::string nova_type           = "mandelbrot"; // "mandelbrot" | "julia"
    double      nova_seed_r         = 1.0;
    double      nova_seed_i         = 0.0;
    int         nova_max_iterations = 256;
    double      nova_tolerance      = 1e-6;
    double      nova_escape_radius  = 1e6;
    float       nova_saturation     = 0.8f;

    // Cyclic Cellular Automaton
    std::string cca_neighborhood    = "moore"; // "moore" | "vonneumann"
    int         cca_states          = 12;
    int         cca_steps           = 500;
    uint32_t    cca_seed            = 42;

    // Physarum Polycephalum (slime mould)
    int      physarum_num_agents     = 200'000;
    int      physarum_steps          = 400;
    float    physarum_sensor_angle   = 45.0f;
    float    physarum_sensor_dist    = 9.0f;
    float    physarum_rotation_angle = 45.0f;
    float    physarum_step_size      = 1.0f;
    float    physarum_deposit        = 5.0f;
    float    physarum_decay          = 0.95f;
    uint32_t physarum_seed           = 42;

    // Post-processing (applied after render)
    PostProcessParams postprocess;

    // Renderer
    int  thread_count  = 0;   // 0 = hardware_concurrency
    int  tile_size     = 64;
    int  aa_samples    = 1;   // 1=off, 2=4spp, 4=16spp
    int  output_dpi    = 300;

    static SceneConfig from_json(const std::string& path);
    std::unique_ptr<IAlgorithm> create_algorithm() const;

    // Build the Palette described by this config (palette_type, stops, phase …).
    // Exposed so AlgorithmRegistry factories can call it without duplicating logic.
    Palette build_palette() const;

    // Optional path to an Adobe Color Swatch (.aco) file; overrides palette_* fields.
    std::string aco_palette_path;
};

} // namespace artgen

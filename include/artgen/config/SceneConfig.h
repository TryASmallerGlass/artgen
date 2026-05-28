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

    // L-System
    std::string ls_preset;     // "plant" "dragon" "sierpinski" "hilbert" "tree"
    std::string ls_axiom;
    std::string ls_rules;      // semicolon-separated "X=successor;Y=successor"
    int         ls_iterations  = 5;
    float       ls_angle       = 25.7f;
    std::string ls_fg_color    = "#33CC55";
    std::string ls_bg_color    = "#080D08";

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

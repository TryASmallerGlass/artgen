#include "artgen/OutputNaming.h"
#include <nlohmann/json.hpp>
#include <chrono>
#include <cstdio>
#include <ctime>
#include <filesystem>
#include <fstream>

namespace artgen {

static constexpr const char* SETTINGS_DIR = "ImageSettings";

std::string make_output_stem(const std::string& algo_name) {
    std::string prefix = algo_name.substr(0, std::min<size_t>(algo_name.size(), 7));

    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
#ifdef _WIN32
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char ts[16];
    std::strftime(ts, sizeof(ts), "%Y%m%d_%H%M", &tm_buf);

    std::error_code ec;
    std::filesystem::create_directories(SETTINGS_DIR, ec);
    int counter = 1;
    if (ec) {
        std::fprintf(stderr, "Warning: could not create %s directory: %s"
                             " — counter will start at 1\n", SETTINGS_DIR, ec.message().c_str());
    } else {
        const std::string counter_file = std::string(SETTINGS_DIR) + "/.counter";
        { std::ifstream cf(counter_file); if (cf) cf >> counter; }
        { std::ofstream cf(counter_file); cf << (counter + 1); }
    }

    char stem[128];
    std::snprintf(stem, sizeof(stem), "%s_%s_%05d", prefix.c_str(), ts, counter);
    return stem;
}

void save_settings_json(const SceneConfig& cfg, const std::string& stem) {
    std::error_code ec;
    std::filesystem::create_directories(SETTINGS_DIR, ec);
    if (ec) {
        std::fprintf(stderr, "Warning: could not create %s directory: %s\n",
                     SETTINGS_DIR, ec.message().c_str());
        return;
    }
    std::string dest = std::string(SETTINGS_DIR) + "/" + stem + ".json";

    nlohmann::json j;
    j["algorithm"]  = cfg.algorithm_name;
    j["output"]     = cfg.output_path;
    j["bit_depth"]  = cfg.bit_depth;
    j["dpi"]        = cfg.output_dpi;
    j["threads"]    = cfg.thread_count;
    j["tile_size"]  = cfg.tile_size;
    j["aa"]         = cfg.aa_samples;
    j["viewport"]   = {
        {"width",    cfg.viewport.pixel_width},
        {"height",   cfg.viewport.pixel_height},
        {"real_min", cfg.viewport.real_min},
        {"real_max", cfg.viewport.real_max},
        {"imag_min", cfg.viewport.imag_min},
        {"imag_max", cfg.viewport.imag_max}
    };
    j["max_iterations"]  = cfg.max_iterations;
    j["escape_radius"]   = cfg.escape_radius;
    j["smooth_coloring"] = cfg.smooth_coloring;
    j["color_cycle"]     = cfg.color_cycle;
    j["coloring_mode"]   = cfg.coloring_mode;
    j["palette"]         = cfg.palette_name;
    j["palette_type"]    = cfg.palette_type;
    j["palette_lab"]     = cfg.palette_lab;
    j["palette_phase"]   = cfg.palette_phase;
    j["palette_hsl"]     = {cfg.palette_hsl_h, cfg.palette_hsl_s, cfg.palette_hsl_l};
    if (!cfg.palette_custom_stops.empty()) {
        nlohmann::json stops = nlohmann::json::array();
        for (const auto& [pos, hex] : cfg.palette_custom_stops)
            stops.push_back({{"pos", pos}, {"color", hex}});
        j["palette_stops"] = stops;
    }
    if (!cfg.aco_palette_path.empty()) j["aco_palette"] = cfg.aco_palette_path;
    j["julia_cr"] = cfg.julia_cr;
    j["julia_ci"] = cfg.julia_ci;
    j["newton_power"]      = cfg.newton_power;
    j["newton_tolerance"]  = cfg.newton_tolerance;
    j["newton_saturation"] = cfg.newton_saturation;
    j["noise_octaves"]     = cfg.noise_octaves;
    j["noise_persistence"] = cfg.noise_persistence;
    j["noise_lacunarity"]  = cfg.noise_lacunarity;
    j["noise_scale"]       = cfg.noise_scale;
    j["noise_seed"]        = cfg.noise_seed;
    j["rd_Du"]     = cfg.rd_Du;
    j["rd_Dv"]     = cfg.rd_Dv;
    j["rd_feed"]   = cfg.rd_feed;
    j["rd_kill"]   = cfg.rd_kill;
    j["rd_dt"]     = cfg.rd_dt;
    j["rd_steps"]  = cfg.rd_steps;
    j["rd_seed"]   = cfg.rd_seed;
    if (!cfg.rd_preset.empty()) j["rd_preset"] = cfg.rd_preset;
    j["multibrot_power"]       = cfg.multibrot_power;
    j["attractor_type"]        = cfg.attractor_type;
    j["attractor_a"]           = cfg.attractor_a;
    j["attractor_b"]           = cfg.attractor_b;
    j["attractor_c"]           = cfg.attractor_c;
    j["attractor_d"]           = cfg.attractor_d;
    j["attractor_u"]           = cfg.attractor_u;
    j["attractor_iterations"]  = cfg.attractor_iterations;
    j["plasma_roughness"] = cfg.plasma_roughness;
    j["plasma_seed"]      = cfg.plasma_seed;
    j["plasma_octaves"]   = cfg.plasma_octaves;
    j["voronoi_seeds"]    = cfg.voronoi_num_seeds;
    j["voronoi_seed"]     = cfg.voronoi_seed;
    j["voronoi_mode"]     = cfg.voronoi_mode;
    j["voronoi_metric"]   = cfg.voronoi_metric;
    if (!cfg.ls_preset.empty())  j["ls_preset"]     = cfg.ls_preset;
    if (!cfg.ls_axiom.empty())   j["ls_axiom"]      = cfg.ls_axiom;
    if (!cfg.ls_rules.empty())   j["ls_rules"]      = cfg.ls_rules;
    j["ls_iterations"] = cfg.ls_iterations;
    j["ls_angle"]      = cfg.ls_angle;
    j["ls_fg_color"]   = cfg.ls_fg_color;
    j["ls_bg_color"]   = cfg.ls_bg_color;
    j["lyapunov_sequence"]   = cfg.lyapunov_sequence;
    j["lyapunov_warmup"]     = cfg.lyapunov_warmup;
    j["lyapunov_iterations"] = cfg.lyapunov_iterations;
    j["lyapunov_seed_x"]     = cfg.lyapunov_seed_x;
    j["nova_power"]          = cfg.nova_power;
    j["nova_relaxation"]     = cfg.nova_relaxation;
    j["nova_type"]           = cfg.nova_type;
    j["nova_seed_r"]         = cfg.nova_seed_r;
    j["nova_seed_i"]         = cfg.nova_seed_i;
    j["nova_max_iterations"] = cfg.nova_max_iterations;
    j["nova_tolerance"]      = cfg.nova_tolerance;
    j["nova_escape_radius"]  = cfg.nova_escape_radius;
    j["nova_saturation"]     = cfg.nova_saturation;
    j["cca_neighborhood"] = cfg.cca_neighborhood;
    j["cca_states"]       = cfg.cca_states;
    j["cca_steps"]        = cfg.cca_steps;
    j["cca_seed"]         = cfg.cca_seed;
    j["physarum_num_agents"]     = cfg.physarum_num_agents;
    j["physarum_steps"]          = cfg.physarum_steps;
    j["physarum_sensor_angle"]   = cfg.physarum_sensor_angle;
    j["physarum_sensor_dist"]    = cfg.physarum_sensor_dist;
    j["physarum_rotation_angle"] = cfg.physarum_rotation_angle;
    j["physarum_step_size"]      = cfg.physarum_step_size;
    j["physarum_deposit"]        = cfg.physarum_deposit;
    j["physarum_decay"]          = cfg.physarum_decay;
    j["physarum_seed"]           = cfg.physarum_seed;
    j["postprocess"] = {
        {"gamma",      cfg.postprocess.gamma},
        {"contrast",   cfg.postprocess.contrast},
        {"brightness", cfg.postprocess.brightness},
        {"saturation", cfg.postprocess.saturation},
        {"vignette",   cfg.postprocess.vignette}
    };

    std::ofstream out(dest);
    if (!out)
        std::fprintf(stderr, "Warning: could not write settings to %s\n", dest.c_str());
    else {
        out << j.dump(2) << '\n';
        std::printf("  Config : %s\n", dest.c_str());
    }
}

void check_render_quality(const PixelBuffer& buf, const std::string& algo_name) {
    const int W = buf.width(), H = buf.height();
    const int stride = 4;

    double sum_lum = 0.0, sum_lsq = 0.0;
    int    n       = 0;
    float  max_lum = 0.0f;

    for (int y = 0; y < H; y += stride) {
        for (int x = 0; x < W; x += stride) {
            RGBA c = buf.get(x, y);
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
    const bool near_black  = (mean_lum < 0.02 && max_lum < 0.05);
    const bool flat_colour = (variance < 5e-5 && mean_lum > 0.02);
    if (!near_black && !flat_colour) return;

    std::fprintf(stderr, "\n  *** Quality warning [%s] ***\n", algo_name.c_str());

    if (near_black) {
        std::fprintf(stderr,
            "  Output is near-black (mean luminance %.4f, max %.4f).\n"
            "  The parameter combination produced no visible structure.\n",
            mean_lum, max_lum);
        if (algo_name == "ikeda")
            std::fprintf(stderr,
                "  Hint: attractor_u < 0.75 produces periodic orbits. "
                "Set attractor_u in [0.75, 0.90].\n");
        else if (algo_name == "lyapunov")
            std::fprintf(stderr,
                "  Hint: viewport must stay within (2,4)×(2,4) and sequence "
                "must contain both 'A' and 'B'.\n");
        else if (algo_name == "attractor")
            std::fprintf(stderr,
                "  Hint: orbit may fall outside viewport. Widen or adjust a/b/c/d.\n");
        else if (algo_name == "nova")
            std::fprintf(stderr,
                "  Hint: try reducing nova_escape_radius or increasing nova_max_iterations.\n");
        else
            std::fprintf(stderr,
                "  Hint: review parameter ranges — see docs/algorithms.md.\n");
    }
    if (flat_colour)
        std::fprintf(stderr,
            "  Output is a flat uniform colour (variance %.2e, mean %.4f).\n"
            "  The algorithm produced no spatial variation — check parameter ranges.\n",
            variance, mean_lum);

    std::fprintf(stderr, "\n");
}

} // namespace artgen

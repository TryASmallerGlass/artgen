// GUI implementation: Dear ImGui + SDL2 + SDL_Renderer

#include "artgen/gui/Gui.h"
#include "artgen/gui/UiStrings.h"
#include "artgen/config/SceneConfig.h"
#include "artgen/AlgorithmRegistry.h"
#include "artgen/OutputNaming.h"
#include "artgen/PixelBuffer.h"
#include "artgen/PostProcess.h"
#include "artgen/Renderer.h"
#include "artgen/output/PngWriter.h"
#include "artgen/output/TiffWriter.h"
#include "artgen/output/ExrWriter.h"

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_sdlrenderer2.h"

#include <SDL.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <commdlg.h>
#else
#include <array>
#include <cstdlib>
#include <cstring>
#endif

namespace artgen::gui {

// ── File dialog helpers ──────────────────────────────────────────────────────

#ifndef _WIN32
static bool command_exists(const char* cmd) {
    std::string check = std::string("command -v ") + cmd + " >/dev/null 2>&1";
    return std::system(check.c_str()) == 0;
}

static std::string run_and_capture(const std::string& cmd) {
    std::array<char, 1024> buf{};
    std::string result;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return {};
    while (fgets(buf.data(), buf.size(), pipe)) result += buf.data();
    pclose(pipe);
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();
    return result;
}

// Pulls the first "*.ext" glob out of a Windows-style double-NUL filter string
// (e.g. "JSON Files\0*.json\0All Files\0*.*\0") for use with zenity/kdialog.
static std::string first_glob_pattern(const char* filter) {
    for (const char* p = filter; *p; ) {
        size_t len = std::strlen(p);
        if (len > 1 && p[0] == '*' && p[1] == '.') return std::string(p, len);
        p += len + 1;
    }
    return "*";
}
#endif

static std::string open_file_dialog(const char* filter) {
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn{};
    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFile   = buf;
    ofn.nMaxFile    = MAX_PATH;
    ofn.lpstrFilter = filter;
    ofn.Flags       = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
    if (GetOpenFileNameA(&ofn)) return buf;
    return {};
#else
    std::string glob = first_glob_pattern(filter);
    if (command_exists("zenity"))
        return run_and_capture("zenity --file-selection --file-filter='" + glob + "' 2>/dev/null");
    if (command_exists("kdialog"))
        return run_and_capture("kdialog --getopenfilename . '" + glob + "' 2>/dev/null");
    std::fprintf(stderr, "Warning: no file picker found (install zenity or kdialog)\n");
    return {};
#endif
}

static std::string save_file_dialog(const char* filter, const char* defext) {
#ifdef _WIN32
    char buf[MAX_PATH] = {};
    OPENFILENAMEA ofn{};
    ofn.lStructSize  = sizeof(ofn);
    ofn.lpstrFile    = buf;
    ofn.nMaxFile     = MAX_PATH;
    ofn.lpstrFilter  = filter;
    ofn.lpstrDefExt  = defext;
    ofn.Flags        = OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;
    if (GetSaveFileNameA(&ofn)) return buf;
    return {};
#else
    std::string glob = first_glob_pattern(filter);
    std::string default_name = std::string("untitled.") + defext;
    if (command_exists("zenity")) {
        std::string p = run_and_capture("zenity --file-selection --save --confirm-overwrite "
                                         "--filename='" + default_name + "' --file-filter='" +
                                         glob + "' 2>/dev/null");
        if (!p.empty() && std::filesystem::path(p).extension().empty()) p += std::string(".") + defext;
        return p;
    }
    if (command_exists("kdialog")) {
        std::string p = run_and_capture("kdialog --getsavefilename " + default_name +
                                         " '" + glob + "' 2>/dev/null");
        if (!p.empty() && std::filesystem::path(p).extension().empty()) p += std::string(".") + defext;
        return p;
    }
    std::fprintf(stderr, "Warning: no file picker found (install zenity or kdialog)\n");
    return {};
#endif
}

// ── Render job ───────────────────────────────────────────────────────────────

struct RenderJob {
    std::thread          thread;
    std::atomic<bool>    running{false};
    std::atomic<bool>    cancel{false};
    std::atomic<int>     done{0};
    std::atomic<int>     total{1};
    std::string          status_msg;
    std::mutex           result_mtx;
    std::unique_ptr<PixelBuffer> result;
    std::string          quality_warning;

    void wait() { if (thread.joinable()) thread.join(); }
};

// ── Preview texture helper ───────────────────────────────────────────────────

static SDL_Texture* upload_to_texture(SDL_Renderer* ren, SDL_Texture* existing,
                                       const PixelBuffer& buf) {
    int W = buf.width(), H = buf.height();
    if (existing) SDL_DestroyTexture(existing);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_STREAMING, W, H);
    if (!tex) return nullptr;
    void* pixels; int pitch;
    SDL_LockTexture(tex, nullptr, &pixels, &pitch);
    uint8_t* dst = static_cast<uint8_t*>(pixels);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            RGBA c = buf.get(x, y);
            uint8_t* p = dst + y * pitch + x * 4;
            p[0] = static_cast<uint8_t>(std::clamp(c.r, 0.f, 1.f) * 255.f);
            p[1] = static_cast<uint8_t>(std::clamp(c.g, 0.f, 1.f) * 255.f);
            p[2] = static_cast<uint8_t>(std::clamp(c.b, 0.f, 1.f) * 255.f);
            p[3] = 255;
        }
    }
    SDL_UnlockTexture(tex);
    return tex;
}

// ── Quality check (same thresholds as CLI) ───────────────────────────────────

static std::string quality_check(const PixelBuffer& buf) {
    int W = buf.width(), H = buf.height();
    double sum_lum = 0, sum_lsq = 0;
    float  max_lum = 0;
    int    n       = 0;
    for (int y = 0; y < H; y += 4)
        for (int x = 0; x < W; x += 4) {
            RGBA c = buf.get(x, y);
            float lum = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
            sum_lum += lum; sum_lsq += lum * lum;
            if (lum > max_lum) max_lum = lum;
            ++n;
        }
    if (n == 0) return {};
    double mean = sum_lum / n;
    double var  = (sum_lsq / n) - (mean * mean);
    if (mean < 0.02 && max_lum < 0.05)
        return "Near-black output — check parameters (see docs/algorithms.md)";
    if (var < 5e-5 && mean > 0.02)
        return "Flat uniform colour — algorithm produced no spatial variation";
    return {};
}

// ── UI helpers ───────────────────────────────────────────────────────────────

static bool g_tips_enabled = true;

static void tip(const char* text) {
    if (!g_tips_enabled || !text || !text[0]) return;
    if (!ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort)) return;

    constexpr float MIN_W = 180.f;
    float wrap_w = std::max(MIN_W, ImGui::GetIO().DisplaySize.x * (2.f / 3.f));

    ImGui::BeginTooltip();
    ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap_w);
    ImGui::TextUnformatted(text);
    ImGui::PopTextWrapPos();
    ImGui::EndTooltip();
}

// Draw a simple schematic flag for a known country code.
// Flags are drawn as coloured stripes using ImDrawList into a w×h rect at pos.
struct Stripe { uint32_t col; float frac; };  // frac = fraction of height (horiz) or width (vert)

static void draw_flag(ImDrawList* dl, ImVec2 pos, float w, float h,
                      const std::string& code) {
    auto abgr = [](uint8_t r, uint8_t g, uint8_t b) -> uint32_t {
        return IM_COL32(r, g, b, 255);
    };

    // Helper: draw N equal horizontal stripes top-to-bottom
    auto horiz = [&](std::initializer_list<uint32_t> cols) {
        float sh = h / static_cast<float>(cols.size());
        float y  = pos.y;
        for (auto c : cols) {
            dl->AddRectFilled({pos.x, y}, {pos.x + w, y + sh}, c);
            y += sh;
        }
    };
    // Helper: draw N equal vertical stripes left-to-right
    auto vert = [&](std::initializer_list<uint32_t> cols) {
        float sw = w / static_cast<float>(cols.size());
        float x  = pos.x;
        for (auto c : cols) {
            dl->AddRectFilled({x, pos.y}, {x + sw, pos.y + h}, c);
            x += sw;
        }
    };

    // Known flags
    if (code == "gb") {
        // Union Jack — approximate with a blue background and red cross
        dl->AddRectFilled(pos, {pos.x + w, pos.y + h}, abgr(0, 36, 125));
        // White diagonal cross (saltire)
        dl->AddLine({pos.x, pos.y}, {pos.x + w, pos.y + h}, abgr(255,255,255), 2.f);
        dl->AddLine({pos.x + w, pos.y}, {pos.x, pos.y + h}, abgr(255,255,255), 2.f);
        // Red diagonal cross
        dl->AddLine({pos.x, pos.y}, {pos.x + w, pos.y + h}, abgr(207,20,43), 1.f);
        dl->AddLine({pos.x + w, pos.y}, {pos.x, pos.y + h}, abgr(207,20,43), 1.f);
        // White cross
        float mx = pos.x + w * 0.5f, my = pos.y + h * 0.5f;
        dl->AddRectFilled({pos.x, my - h*0.18f}, {pos.x + w, my + h*0.18f}, abgr(255,255,255));
        dl->AddRectFilled({mx - w*0.18f, pos.y}, {mx + w*0.18f, pos.y + h}, abgr(255,255,255));
        // Red cross
        dl->AddRectFilled({pos.x, my - h*0.12f}, {pos.x + w, my + h*0.12f}, abgr(207,20,43));
        dl->AddRectFilled({mx - w*0.12f, pos.y}, {mx + w*0.12f, pos.y + h}, abgr(207,20,43));
    } else if (code == "fr") {
        vert({abgr(0,35,149), abgr(255,255,255), abgr(237,41,57)});
    } else if (code == "de") {
        horiz({abgr(0,0,0), abgr(221,0,0), abgr(255,206,0)});
    } else if (code == "es") {
        horiz({abgr(170,21,27), abgr(241,191,0), abgr(170,21,27)});
    } else if (code == "it") {
        vert({abgr(0,146,70), abgr(255,255,255), abgr(206,43,55)});
    } else if (code == "pt") {
        // Green/red split (approximate)
        dl->AddRectFilled(pos, {pos.x + w*0.4f, pos.y + h}, abgr(0,102,0));
        dl->AddRectFilled({pos.x + w*0.4f, pos.y}, {pos.x + w, pos.y + h}, abgr(255,0,0));
    } else if (code == "nl") {
        horiz({abgr(174,28,40), abgr(255,255,255), abgr(33,70,139)});
    } else if (code == "pl") {
        horiz({abgr(255,255,255), abgr(220,20,60)});
    } else if (code == "us") {
        // Simplified: red/white stripes with blue canton
        for (int i = 0; i < 7; ++i) {
            uint32_t c = (i % 2 == 0) ? abgr(178,34,52) : abgr(255,255,255);
            dl->AddRectFilled({pos.x, pos.y + i*(h/7.f)},
                              {pos.x + w, pos.y + (i+1)*(h/7.f)}, c);
        }
        dl->AddRectFilled(pos, {pos.x + w*0.4f, pos.y + h*0.5f}, abgr(60,59,110));
    } else if (code == "jp") {
        dl->AddRectFilled(pos, {pos.x+w, pos.y+h}, abgr(255,255,255));
        dl->AddCircleFilled({pos.x+w*0.5f, pos.y+h*0.5f}, h*0.3f, abgr(188,0,45));
    } else if (code == "cn") {
        dl->AddRectFilled(pos, {pos.x+w, pos.y+h}, abgr(222,41,16));
        dl->AddCircleFilled({pos.x+w*0.2f, pos.y+h*0.35f}, h*0.18f, abgr(255,222,0));
    } else {
        // Generic: grey box with the two-letter code
        dl->AddRectFilled(pos, {pos.x+w, pos.y+h}, abgr(120,120,120));
        // No easy way to draw text here without font; just a placeholder box
        dl->AddRect(pos, {pos.x+w, pos.y+h}, abgr(200,200,200));
    }

    // Border
    dl->AddRect(pos, {pos.x+w, pos.y+h}, abgr(80,80,80));
}

// ── Palette gradient preview texture (1×256 strip, horizontal) ──────────────

static SDL_Texture* make_palette_tex(SDL_Renderer* ren, SDL_Texture* existing,
                                      const SceneConfig& cfg) {
    constexpr int W = 256, H = 1;
    if (existing) SDL_DestroyTexture(existing);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA32,
                                          SDL_TEXTUREACCESS_STREAMING, W, H);
    if (!tex) return nullptr;
    Palette pal = cfg.build_palette();
    void* pixels; int pitch;
    SDL_LockTexture(tex, nullptr, &pixels, &pitch);
    uint8_t* dst = static_cast<uint8_t*>(pixels);
    for (int x = 0; x < W; ++x) {
        float t = static_cast<float>(x) / (W - 1);
        RGBA c  = pal.sample(t);
        dst[x * 4 + 0] = static_cast<uint8_t>(std::clamp(c.r, 0.f, 1.f) * 255);
        dst[x * 4 + 1] = static_cast<uint8_t>(std::clamp(c.g, 0.f, 1.f) * 255);
        dst[x * 4 + 2] = static_cast<uint8_t>(std::clamp(c.b, 0.f, 1.f) * 255);
        dst[x * 4 + 3] = 255;
    }
    SDL_UnlockTexture(tex);
    return tex;
}

// ── Algorithm parameter panel ────────────────────────────────────────────────

static void draw_algo_params(SceneConfig& cfg, const UiStrings& S) {
    const std::string& a = cfg.algorithm_name;

    if (a == "mandelbrot" || a == "julia" || a == "burning_ship" ||
        a == "multibrot"  || a == "tricorn") {
        ImGui::SliderInt(S.label("max_iterations"), &cfg.max_iterations, 100, 10000);
        tip(S.tooltip("max_iterations"));
        float er = static_cast<float>(cfg.escape_radius);
        if (ImGui::SliderFloat(S.label("escape_radius"), &er, 2.0f, 100.0f))
            cfg.escape_radius = er;
        tip(S.tooltip("escape_radius"));
        ImGui::Checkbox(S.label("smooth_coloring"), &cfg.smooth_coloring);
        tip(S.tooltip("smooth_coloring"));
        float cc = static_cast<float>(cfg.color_cycle);
        if (ImGui::SliderFloat(S.label("color_cycle"), &cc, 1.0f, 256.0f))
            cfg.color_cycle = cc;
        tip(S.tooltip("color_cycle"));

        const char* modes[] = {"smooth", "histogram_eq", "orbit_trap"};
        int cm = 0;
        for (int i = 0; i < 3; ++i) if (cfg.coloring_mode == modes[i]) cm = i;
        if (ImGui::Combo(S.label("coloring_mode"), &cm, modes, 3))
            cfg.coloring_mode = modes[cm];
        tip(S.tooltip("coloring_mode"));

        if (a == "julia") {
            float cr = static_cast<float>(cfg.julia_cr);
            float ci = static_cast<float>(cfg.julia_ci);
            if (ImGui::SliderFloat(S.label("julia_cr"), &cr, -2.f, 2.f)) cfg.julia_cr = cr;
            tip(S.tooltip("julia_cr"));
            if (ImGui::SliderFloat(S.label("julia_ci"), &ci, -2.f, 2.f)) cfg.julia_ci = ci;
            tip(S.tooltip("julia_ci"));
        }
        if (a == "multibrot") {
            float mp = static_cast<float>(cfg.multibrot_power);
            if (ImGui::SliderFloat(S.label("multibrot_power"), &mp, 2.f, 8.f))
                cfg.multibrot_power = mp;
            tip(S.tooltip("multibrot_power"));
        }
    }

    if (a == "newton") {
        ImGui::SliderInt(S.label("newton_power"), &cfg.newton_power, 2, 8);
        tip(S.tooltip("newton_power"));
        ImGui::SliderFloat(S.label("newton_saturation"), &cfg.newton_saturation, 0.f, 1.f);
        tip(S.tooltip("newton_saturation"));
    }

    if (a == "nova") {
        const char* types[] = {"mandelbrot", "julia"};
        int nt = (cfg.nova_type == "julia") ? 1 : 0;
        if (ImGui::Combo(S.label("nova_type"), &nt, types, 2))
            cfg.nova_type = types[nt];
        tip(S.tooltip("nova_type"));
        ImGui::SliderInt(S.label("nova_power"), &cfg.nova_power, 2, 8);
        tip(S.tooltip("nova_power"));
        float nr = static_cast<float>(cfg.nova_relaxation);
        if (ImGui::SliderFloat(S.label("nova_relaxation"), &nr, 0.1f, 3.0f))
            cfg.nova_relaxation = nr;
        tip(S.tooltip("nova_relaxation"));
        ImGui::SliderInt(S.label("nova_max_iterations"), &cfg.nova_max_iterations, 64, 1024);
        tip(S.tooltip("nova_max_iterations"));
        float sr = static_cast<float>(cfg.nova_seed_r);
        float si = static_cast<float>(cfg.nova_seed_i);
        if (ImGui::SliderFloat(S.label("nova_seed_r"), &sr, -3.f, 3.f)) cfg.nova_seed_r = sr;
        tip(S.tooltip("nova_seed_r"));
        if (ImGui::SliderFloat(S.label("nova_seed_i"), &si, -3.f, 3.f)) cfg.nova_seed_i = si;
        tip(S.tooltip("nova_seed_i"));
    }

    if (a == "noise") {
        ImGui::SliderInt(S.label("noise_octaves"), &cfg.noise_octaves, 1, 12);
        tip(S.tooltip("noise_octaves"));
        ImGui::SliderFloat(S.label("noise_persistence"), &cfg.noise_persistence, 0.1f, 1.0f);
        tip(S.tooltip("noise_persistence"));
        ImGui::SliderFloat(S.label("noise_lacunarity"), &cfg.noise_lacunarity, 1.0f, 4.0f);
        tip(S.tooltip("noise_lacunarity"));
        ImGui::SliderFloat(S.label("noise_scale"), &cfg.noise_scale, 0.1f, 10.0f);
        tip(S.tooltip("noise_scale"));
        int ns = static_cast<int>(cfg.noise_seed);
        if (ImGui::SliderInt(S.label("noise_seed"), &ns, 0, 9999))
            cfg.noise_seed = static_cast<uint32_t>(ns);
        tip(S.tooltip("noise_seed"));
    }

    if (a == "reaction_diffusion") {
        const char* presets[] = {"(manual)", "coral", "mitosis", "worms",
                                  "maze", "spots", "fingerprint"};
        int pidx = 0;
        for (int i = 1; i < 7; ++i)
            if (cfg.rd_preset == presets[i]) { pidx = i; break; }
        if (ImGui::Combo(S.label("rd_preset"), &pidx, presets, 7))
            cfg.rd_preset = (pidx == 0) ? "" : presets[pidx];
        tip(S.tooltip("rd_preset"));
        ImGui::SliderFloat(S.label("rd_feed"), &cfg.rd_feed, 0.01f, 0.10f, "%.4f");
        tip(S.tooltip("rd_feed"));
        ImGui::SliderFloat(S.label("rd_kill"), &cfg.rd_kill, 0.04f, 0.07f, "%.4f");
        tip(S.tooltip("rd_kill"));
        ImGui::SliderInt(S.label("rd_steps"), &cfg.rd_steps, 1000, 20000);
        tip(S.tooltip("rd_steps"));
        int rs = static_cast<int>(cfg.rd_seed);
        if (ImGui::SliderInt(S.label("rd_seed"), &rs, 0, 9999))
            cfg.rd_seed = static_cast<uint32_t>(rs);
        tip(S.tooltip("rd_seed"));
    }

    if (a == "plasma") {
        ImGui::SliderFloat(S.label("plasma_roughness"), &cfg.plasma_roughness, 0.0f, 1.0f);
        tip(S.tooltip("plasma_roughness"));
        ImGui::SliderInt(S.label("plasma_octaves"), &cfg.plasma_octaves, 4, 12);
        tip(S.tooltip("plasma_octaves"));
        int ps = static_cast<int>(cfg.plasma_seed);
        if (ImGui::SliderInt(S.label("plasma_seed"), &ps, 0, 9999))
            cfg.plasma_seed = static_cast<uint32_t>(ps);
        tip(S.tooltip("plasma_seed"));
    }

    if (a == "attractor") {
        const char* atypes[] = {"clifford", "dejong"};
        int at = (cfg.attractor_type == "dejong") ? 1 : 0;
        if (ImGui::Combo(S.label("attractor_type"), &at, atypes, 2))
            cfg.attractor_type = atypes[at];
        tip(S.tooltip("attractor_type"));
        float aa = static_cast<float>(cfg.attractor_a);
        float ab = static_cast<float>(cfg.attractor_b);
        float ac = static_cast<float>(cfg.attractor_c);
        float ad = static_cast<float>(cfg.attractor_d);
        if (ImGui::SliderFloat(S.label("attractor_a"), &aa, -3.f, 3.f)) cfg.attractor_a = aa;
        tip(S.tooltip("attractor_a"));
        if (ImGui::SliderFloat(S.label("attractor_b"), &ab, -3.f, 3.f)) cfg.attractor_b = ab;
        tip(S.tooltip("attractor_b"));
        if (ImGui::SliderFloat(S.label("attractor_c"), &ac, -3.f, 3.f)) cfg.attractor_c = ac;
        tip(S.tooltip("attractor_c"));
        if (ImGui::SliderFloat(S.label("attractor_d"), &ad, -3.f, 3.f)) cfg.attractor_d = ad;
        tip(S.tooltip("attractor_d"));
        int ai = cfg.attractor_iterations / 1000000;
        if (ImGui::SliderInt(S.label("attractor_iterations"), &ai, 1, 20))
            cfg.attractor_iterations = ai * 1000000;
        tip(S.tooltip("attractor_iterations"));
    }

    if (a == "ikeda") {
        float u = static_cast<float>(cfg.attractor_u);
        if (ImGui::SliderFloat(S.label("ikeda_u"), &u, 0.5f, 0.99f))
            cfg.attractor_u = u;
        tip(S.tooltip("ikeda_u"));
        int ai = cfg.attractor_iterations / 1000000;
        if (ImGui::SliderInt(S.label("ikeda_iterations"), &ai, 1, 20))
            cfg.attractor_iterations = ai * 1000000;
        tip(S.tooltip("ikeda_iterations"));
    }

    if (a == "voronoi") {
        ImGui::SliderInt(S.label("voronoi_seeds"), &cfg.voronoi_num_seeds, 4, 512);
        tip(S.tooltip("voronoi_seeds"));
        const char* vmodes[] = {"cells", "distance", "edge"};
        int vm = 0;
        for (int i = 0; i < 3; ++i) if (cfg.voronoi_mode == vmodes[i]) vm = i;
        if (ImGui::Combo(S.label("voronoi_mode"), &vm, vmodes, 3))
            cfg.voronoi_mode = vmodes[vm];
        tip(S.tooltip("voronoi_mode"));
        const char* metrics[] = {"euclidean", "manhattan", "chebyshev"};
        int vmet = 0;
        for (int i = 0; i < 3; ++i) if (cfg.voronoi_metric == metrics[i]) vmet = i;
        if (ImGui::Combo(S.label("voronoi_metric"), &vmet, metrics, 3))
            cfg.voronoi_metric = metrics[vmet];
        tip(S.tooltip("voronoi_metric"));
        int vs = static_cast<int>(cfg.voronoi_seed);
        if (ImGui::SliderInt(S.label("voronoi_seed"), &vs, 0, 9999))
            cfg.voronoi_seed = static_cast<uint32_t>(vs);
        tip(S.tooltip("voronoi_seed"));
    }

    if (a == "lsystem") {
        const char* lpresets[] = {"(custom)", "plant", "dragon", "sierpinski",
                                   "hilbert", "tree"};
        int lpi = 0;
        for (int i = 1; i < 6; ++i)
            if (cfg.ls_preset == lpresets[i]) { lpi = i; break; }
        if (ImGui::Combo(S.label("ls_preset"), &lpi, lpresets, 6))
            cfg.ls_preset = (lpi == 0) ? "" : lpresets[lpi];
        tip(S.tooltip("ls_preset"));
        ImGui::SliderInt(S.label("ls_iterations"), &cfg.ls_iterations, 1, 10);
        tip(S.tooltip("ls_iterations"));
        ImGui::SliderFloat(S.label("ls_angle"), &cfg.ls_angle, 1.f, 90.f);
        tip(S.tooltip("ls_angle"));
    }

    if (a == "lyapunov") {
        static char seq_buf[64];
        std::snprintf(seq_buf, sizeof(seq_buf), "%s", cfg.lyapunov_sequence.c_str());
        if (ImGui::InputText(S.label("lyapunov_sequence"), seq_buf, sizeof(seq_buf)))
            cfg.lyapunov_sequence = seq_buf;
        tip(S.tooltip("lyapunov_sequence"));
        ImGui::SliderInt(S.label("lyapunov_warmup"), &cfg.lyapunov_warmup, 50, 500);
        tip(S.tooltip("lyapunov_warmup"));
        ImGui::SliderInt(S.label("lyapunov_iterations"), &cfg.lyapunov_iterations, 100, 5000);
        tip(S.tooltip("lyapunov_iterations"));
    }

    if (a == "cyclic_ca") {
        ImGui::SliderInt(S.label("cca_states"), &cfg.cca_states, 2, 32);
        tip(S.tooltip("cca_states"));
        ImGui::SliderInt(S.label("cca_steps"), &cfg.cca_steps, 50, 2000);
        tip(S.tooltip("cca_steps"));
        const char* nhoods[] = {"moore", "vonneumann"};
        int ni = (cfg.cca_neighborhood == "vonneumann") ? 1 : 0;
        if (ImGui::Combo(S.label("cca_neighborhood"), &ni, nhoods, 2))
            cfg.cca_neighborhood = nhoods[ni];
        tip(S.tooltip("cca_neighborhood"));
        int cs = static_cast<int>(cfg.cca_seed);
        if (ImGui::SliderInt(S.label("cca_seed"), &cs, 0, 9999))
            cfg.cca_seed = static_cast<uint32_t>(cs);
        tip(S.tooltip("cca_seed"));
    }

    if (a == "physarum") {
        ImGui::SliderInt(S.label("physarum_agents"), &cfg.physarum_num_agents, 10000, 500000);
        tip(S.tooltip("physarum_agents"));
        ImGui::SliderInt(S.label("physarum_steps"), &cfg.physarum_steps, 50, 2000);
        tip(S.tooltip("physarum_steps"));
        ImGui::SliderFloat(S.label("physarum_sensor_angle"), &cfg.physarum_sensor_angle, 10.f, 90.f);
        tip(S.tooltip("physarum_sensor_angle"));
        ImGui::SliderFloat(S.label("physarum_sensor_dist"), &cfg.physarum_sensor_dist, 1.f, 50.f);
        tip(S.tooltip("physarum_sensor_dist"));
        ImGui::SliderFloat(S.label("physarum_rotation_angle"), &cfg.physarum_rotation_angle, 5.f, 90.f);
        tip(S.tooltip("physarum_rotation_angle"));
        ImGui::SliderFloat(S.label("physarum_step_size"), &cfg.physarum_step_size, 0.1f, 5.f);
        tip(S.tooltip("physarum_step_size"));
        ImGui::SliderFloat(S.label("physarum_deposit"), &cfg.physarum_deposit, 0.1f, 20.f);
        tip(S.tooltip("physarum_deposit"));
        ImGui::SliderFloat(S.label("physarum_decay"), &cfg.physarum_decay, 0.8f, 0.999f);
        tip(S.tooltip("physarum_decay"));
        int phs = static_cast<int>(cfg.physarum_seed);
        if (ImGui::SliderInt(S.label("physarum_seed"), &phs, 0, 9999))
            cfg.physarum_seed = static_cast<uint32_t>(phs);
        tip(S.tooltip("physarum_seed"));
    }
}

// ── Viewport panel ───────────────────────────────────────────────────────────

static void draw_viewport_panel(SceneConfig& cfg, const UiStrings& S) {
    ImGui::SliderInt(S.label("viewport_width"),  &cfg.viewport.pixel_width,  256, 7680);
    tip(S.tooltip("viewport_width"));
    ImGui::SliderInt(S.label("viewport_height"), &cfg.viewport.pixel_height, 256, 4320);
    tip(S.tooltip("viewport_height"));
    float rmin = static_cast<float>(cfg.viewport.real_min);
    float rmax = static_cast<float>(cfg.viewport.real_max);
    float imin = static_cast<float>(cfg.viewport.imag_min);
    float imax = static_cast<float>(cfg.viewport.imag_max);
    if (ImGui::InputFloat(S.label("viewport_real_min"), &rmin, 0, 0, "%.6f")) cfg.viewport.real_min = rmin;
    tip(S.tooltip("viewport_real_min"));
    if (ImGui::InputFloat(S.label("viewport_real_max"), &rmax, 0, 0, "%.6f")) cfg.viewport.real_max = rmax;
    tip(S.tooltip("viewport_real_max"));
    if (ImGui::InputFloat(S.label("viewport_imag_min"), &imin, 0, 0, "%.6f")) cfg.viewport.imag_min = imin;
    tip(S.tooltip("viewport_imag_min"));
    if (ImGui::InputFloat(S.label("viewport_imag_max"), &imax, 0, 0, "%.6f")) cfg.viewport.imag_max = imax;
    tip(S.tooltip("viewport_imag_max"));
    if (ImGui::Button("Reset viewport")) {
        cfg.viewport.real_min = -2.5; cfg.viewport.real_max = 1.0;
        cfg.viewport.imag_min = -1.25; cfg.viewport.imag_max = 1.25;
    }
}

// ── Palette panel ────────────────────────────────────────────────────────────

static void draw_palette_panel(SceneConfig& cfg, SDL_Texture*& pal_tex,
                                 SDL_Renderer* ren, bool& pal_dirty,
                                 const UiStrings& S) {
    const char* ptypes[] = {"named", "complementary", "triadic",
                             "analogous", "split_complementary", "custom"};
    int pi = 0;
    for (int i = 0; i < 6; ++i) if (cfg.palette_type == ptypes[i]) pi = i;
    if (ImGui::Combo(S.label("palette_type"), &pi, ptypes, 6)) {
        cfg.palette_type = ptypes[pi]; pal_dirty = true;
    }
    tip(S.tooltip("palette_type"));

    if (cfg.palette_type == "named") {
        const char* names[] = {"classic_mandelbrot","fire","ice","electric","grayscale"};
        int ni = 0;
        for (int i = 0; i < 5; ++i) if (cfg.palette_name == names[i]) ni = i;
        if (ImGui::Combo(S.label("palette_name"), &ni, names, 5)) {
            cfg.palette_name = names[ni]; pal_dirty = true;
        }
        tip(S.tooltip("palette_name"));
    } else if (cfg.palette_type != "custom") {
        if (ImGui::SliderFloat(S.label("palette_hue"), &cfg.palette_hsl_h, 0.f, 360.f)) pal_dirty = true;
        tip(S.tooltip("palette_hue"));
        if (ImGui::SliderFloat(S.label("palette_saturation"), &cfg.palette_hsl_s, 0.f, 1.f)) pal_dirty = true;
        tip(S.tooltip("palette_saturation"));
        if (ImGui::SliderFloat(S.label("palette_lightness"), &cfg.palette_hsl_l, 0.f, 1.f)) pal_dirty = true;
        tip(S.tooltip("palette_lightness"));
    }

    if (ImGui::SliderFloat(S.label("palette_phase"), &cfg.palette_phase, 0.f, 1.f)) pal_dirty = true;
    tip(S.tooltip("palette_phase"));
    if (ImGui::Checkbox(S.label("palette_lab"), &cfg.palette_lab)) pal_dirty = true;
    tip(S.tooltip("palette_lab"));

    // Gradient bar
    if (pal_dirty) {
        pal_tex  = make_palette_tex(ren, pal_tex, cfg);
        pal_dirty = false;
    }
    if (pal_tex) {
        ImVec2 uv0(0, 0), uv1(1, 1);
        ImGui::Image(reinterpret_cast<ImTextureID>(pal_tex),
                     ImVec2(ImGui::GetContentRegionAvail().x, 20), uv0, uv1);
    }
}

// ── Post-process panel ───────────────────────────────────────────────────────

static void draw_postprocess_panel(SceneConfig& cfg, const UiStrings& S) {
    ImGui::SliderFloat(S.label("pp_gamma"),      &cfg.postprocess.gamma,      0.5f, 3.0f);
    tip(S.tooltip("pp_gamma"));
    ImGui::SliderFloat(S.label("pp_brightness"), &cfg.postprocess.brightness, -1.0f, 1.0f);
    tip(S.tooltip("pp_brightness"));
    ImGui::SliderFloat(S.label("pp_contrast"),   &cfg.postprocess.contrast,    0.0f, 3.0f);
    tip(S.tooltip("pp_contrast"));
    ImGui::SliderFloat(S.label("pp_saturation"), &cfg.postprocess.saturation,  0.0f, 3.0f);
    tip(S.tooltip("pp_saturation"));
    ImGui::SliderFloat(S.label("pp_vignette"),   &cfg.postprocess.vignette,    0.0f, 1.0f);
    tip(S.tooltip("pp_vignette"));
    if (ImGui::Button("Reset PP")) {
        cfg.postprocess = PostProcessParams{};
    }
}

// ── Renderer options ─────────────────────────────────────────────────────────

static void draw_renderer_panel(SceneConfig& cfg, const UiStrings& S) {
    ImGui::SliderInt(S.label("aa_samples"),   &cfg.aa_samples,   1, 4);
    tip(S.tooltip("aa_samples"));
    ImGui::SliderInt(S.label("thread_count"), &cfg.thread_count, 0, 32);
    tip(S.tooltip("thread_count"));
    ImGui::SliderInt(S.label("tile_size"),    &cfg.tile_size,    16, 256);
    tip(S.tooltip("tile_size"));
    ImGui::SliderInt(S.label("output_dpi"),   &cfg.output_dpi,   72, 600);
    tip(S.tooltip("output_dpi"));
    const char* depths[] = {"8-bit", "16-bit"};
    int d = (cfg.bit_depth == 16) ? 1 : 0;
    if (ImGui::Combo(S.label("bit_depth"), &d, depths, 2))
        cfg.bit_depth = (d == 1) ? 16 : 8;
    tip(S.tooltip("bit_depth"));
}

// ── Main GUI entry point ─────────────────────────────────────────────────────

int run() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "artgen GUI",
                                 SDL_GetError(), nullptr);
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow(
        "artgen GUI",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        1400, 900,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* ren = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_ACCELERATED);
    if (!ren)
        ren = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC | SDL_RENDERER_SOFTWARE);
    if (!ren) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "artgen GUI",
                                 SDL_GetError(), nullptr);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();

    ImGui_ImplSDL2_InitForSDLRenderer(window, ren);
    ImGui_ImplSDLRenderer2_Init(ren);

    // ── State ────────────────────────────────────────────────────────────────
    enum class AppMode { MainMenu, GenerativeArt };
    AppMode mode = AppMode::MainMenu;

    auto languages   = scan_languages();
    int  lang_idx    = 0;  // index into languages
    // Default to "en" if present
    for (int i = 0; i < static_cast<int>(languages.size()); ++i)
        if (languages[i].code == "en") { lang_idx = i; break; }

    UiStrings S(languages.empty() ? "en" : languages[lang_idx].code);
    SceneConfig cfg;
    RenderJob   job;
    SDL_Texture* preview_tex  = nullptr;
    SDL_Texture* pal_tex      = nullptr;
    bool         pal_dirty    = true;
    std::atomic<bool> tex_pending{false};  // new result waiting to be uploaded
    std::string  save_path;
    std::string  quality_warn;

    // Pan/zoom state for preview
    float pan_x = 0.f, pan_y = 0.f, zoom = 1.f;

    // Output format selection
    const char* fmt_names[] = {"PNG", "TIFF", "EXR"};
    int fmt_idx = 0;

    // Recent files
    std::vector<std::string> recent_files;

    // Status line
    std::string status_msg = "Ready";

    // Quick-render flag: render at 1/4 resolution
    bool quick_render = true;

    // Algorithm list
    auto algo_names_vec = AlgorithmRegistry::names();
    std::vector<const char*> algo_cstrs;
    for (const auto& s : algo_names_vec) algo_cstrs.push_back(s.c_str());
    int algo_idx = 0;
    for (int i = 0; i < static_cast<int>(algo_names_vec.size()); ++i)
        if (algo_names_vec[i] == cfg.algorithm_name) { algo_idx = i; break; }

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            ImGui_ImplSDL2_ProcessEvent(&ev);
            if (ev.type == SDL_QUIT) running = false;
        }

        // Upload render result when ready
        if (tex_pending.load()) {
            std::lock_guard<std::mutex> lk(job.result_mtx);
            if (job.result) {
                preview_tex = upload_to_texture(ren, preview_tex, *job.result);
                quality_warn = quality_check(*job.result);
                pan_x = 0; pan_y = 0; zoom = 1;
            }
            tex_pending.store(false);
            status_msg  = job.status_msg;
        }

        ImGui_ImplSDLRenderer2_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // ── Full-screen dockspace layout ────────────────────────────────────
        int ww, wh;
        SDL_GetWindowSize(window, &ww, &wh);

        if (mode == AppMode::MainMenu) {
            // ── Main menu screen ─────────────────────────────────────────────
            ImGui::SetNextWindowPos(ImVec2(0, 0));
            ImGui::SetNextWindowSize(ImVec2(static_cast<float>(ww), static_cast<float>(wh)));
            ImGui::Begin("Main Menu", nullptr,
                         ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                         ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

            const ImVec2 btn_size(280, 48);
            float center_x = (static_cast<float>(ww) - btn_size.x) * 0.5f;
            float top_y    = static_cast<float>(wh) * 0.35f;

            auto centered_text = [&](const char* text) {
                float w = ImGui::CalcTextSize(text).x;
                ImGui::SetCursorPosX((static_cast<float>(ww) - w) * 0.5f);
                ImGui::TextUnformatted(text);
            };

            ImGui::SetCursorPosY(top_y - 60);
            centered_text("artgen");

            ImGui::SetCursorPos(ImVec2(center_x, top_y));
            if (ImGui::Button("Generative Art", btn_size)) {
                mode = AppMode::GenerativeArt;
            }

            ImGui::SetCursorPos(ImVec2(center_x, top_y + btn_size.y + 16));
            ImGui::BeginDisabled();
            ImGui::Button("Module 2 (Coming soon)", btn_size);
            ImGui::EndDisabled();

            ImGui::SetCursorPos(ImVec2(center_x, top_y + 2 * (btn_size.y + 16)));
            ImGui::BeginDisabled();
            ImGui::Button("Module 3 (Coming soon)", btn_size);
            ImGui::EndDisabled();

            ImGui::End(); // Main Menu
        } else {
        // ── Left panel: controls ─────────────────────────────────────────────
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(380, static_cast<float>(wh)));
        ImGui::Begin("Controls", nullptr,
                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse);

        if (ImGui::Button("< Main Menu")) {
            mode = AppMode::MainMenu;
        }
        ImGui::Separator();

        // ── Language picker (top-right of controls panel) ────────────────────
        if (!languages.empty()) {
            constexpr float FLAG_W = 22.f, FLAG_H = 14.f, PICKER_W = 130.f;
            constexpr float CHECK_W = 30.f;
            float avail = ImGui::GetContentRegionAvail().x;
            float row_x = avail - CHECK_W - 4.f - PICKER_W + ImGui::GetStyle().WindowPadding.x;

            // Tooltip checkbox first (left of language dropdown)
            ImGui::SetCursorPosX(row_x);
            ImGui::Checkbox("?##tips", &g_tips_enabled);
            if (ImGui::IsItemHovered()) {
                constexpr float MIN_W = 180.f;
                float wrap_w = std::max(MIN_W, ImGui::GetIO().DisplaySize.x * (2.f / 3.f));
                ImGui::BeginTooltip();
                ImGui::PushTextWrapPos(ImGui::GetCursorPosX() + wrap_w);
                ImGui::TextUnformatted("Show / hide tooltips");
                ImGui::PopTextWrapPos();
                ImGui::EndTooltip();
            }

            ImGui::SameLine();

            // Flag drawn into the combo row
            ImDrawList* dl = ImGui::GetWindowDrawList();
            ImVec2 flag_pos = ImGui::GetCursorScreenPos();
            flag_pos.y += (ImGui::GetFrameHeight() - FLAG_H) * 0.5f;
            draw_flag(dl, flag_pos, FLAG_W, FLAG_H, languages[lang_idx].flag);

            ImGui::SetCursorPosX(ImGui::GetCursorPosX() + FLAG_W + 4.f);
            ImGui::SetNextItemWidth(PICKER_W - FLAG_W - 4.f);

            if (ImGui::BeginCombo("##lang", languages[lang_idx].name.c_str())) {
                for (int i = 0; i < static_cast<int>(languages.size()); ++i) {
                    ImVec2 fp = ImGui::GetCursorScreenPos();
                    fp.y += (ImGui::GetFrameHeight() - FLAG_H) * 0.5f;
                    ImGui::Dummy({FLAG_W + 4.f, ImGui::GetFrameHeight()});
                    draw_flag(ImGui::GetWindowDrawList(), fp, FLAG_W, FLAG_H,
                              languages[i].flag);
                    ImGui::SameLine();
                    bool selected = (i == lang_idx);
                    if (ImGui::Selectable(languages[i].name.c_str(), selected)) {
                        lang_idx = i;
                        S.load(languages[i].code);
                    }
                    if (selected) ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            tip("Select display language / Sélectionner la langue");
        }

        ImGui::Separator();

        // Algorithm selector
        if (ImGui::Combo("Algorithm",
                         &algo_idx,
                         algo_cstrs.data(),
                         static_cast<int>(algo_cstrs.size()))) {
            cfg.algorithm_name = algo_names_vec[algo_idx];
        }
        tip(S.algo_tooltip(cfg.algorithm_name.c_str()));

        // File ops
        if (ImGui::Button("Open scene...")) {
            std::string p = open_file_dialog("JSON Files\0*.json\0All Files\0*.*\0");
            if (!p.empty()) {
                try {
                    cfg = SceneConfig::from_json(p);
                    // Sync algo_idx
                    for (int i = 0; i < static_cast<int>(algo_names_vec.size()); ++i)
                        if (algo_names_vec[i] == cfg.algorithm_name) { algo_idx = i; break; }
                    recent_files.erase(
                        std::remove(recent_files.begin(), recent_files.end(), p),
                        recent_files.end());
                    recent_files.insert(recent_files.begin(), p);
                    if (recent_files.size() > 10) recent_files.resize(10);
                    pal_dirty = true;
                    status_msg = "Loaded: " + p;
                } catch (const std::exception& e) {
                    status_msg = std::string("Load error: ") + e.what();
                }
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Save scene...")) {
            std::string p = save_file_dialog("JSON Files\0*.json\0", "json");
            if (!p.empty()) {
                try {
                    std::filesystem::path scene_path(p);
                    std::string stem = scene_path.stem().string();
                    if (stem.empty()) stem = scene_path.filename().string();
                    std::filesystem::path saved_path =
                        std::filesystem::path("ImageSettings") / (stem + ".json");
                    artgen::save_settings_json(cfg, stem);
                    status_msg = "Saved: " + saved_path.string();
                } catch (const std::exception& e) {
                    status_msg = std::string("Save error: ") + e.what();
                }
            }
        }

        // Recent files
        if (!recent_files.empty() && ImGui::BeginMenu("Recent")) {
            for (const auto& f : recent_files) {
                if (ImGui::MenuItem(f.c_str())) {
                    try {
                        cfg = SceneConfig::from_json(f);
                        for (int i = 0; i < static_cast<int>(algo_names_vec.size()); ++i)
                            if (algo_names_vec[i] == cfg.algorithm_name) { algo_idx = i; break; }
                        pal_dirty  = true;
                        status_msg = "Loaded: " + f;
                    } catch (const std::exception& e) {
                        status_msg = std::string("Load error: ") + e.what();
                    }
                }
            }
            ImGui::EndMenu();
        }

        ImGui::Separator();

        // Accordion panels
        if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen))
            draw_viewport_panel(cfg, S);
        if (ImGui::CollapsingHeader("Algorithm Parameters", ImGuiTreeNodeFlags_DefaultOpen))
            draw_algo_params(cfg, S);
        if (ImGui::CollapsingHeader("Palette", ImGuiTreeNodeFlags_DefaultOpen))
            draw_palette_panel(cfg, pal_tex, ren, pal_dirty, S);
        if (ImGui::CollapsingHeader("Post-Process"))
            draw_postprocess_panel(cfg, S);
        if (ImGui::CollapsingHeader("Renderer"))
            draw_renderer_panel(cfg, S);

        ImGui::Separator();

        // Output format
        ImGui::Combo(S.label("output_format"), &fmt_idx, fmt_names, 3);
        tip(S.tooltip("output_format"));
        ImGui::Checkbox(S.label("quick_render"), &quick_render);
        tip(S.tooltip("quick_render"));

        ImGui::Separator();

        // Render / Cancel buttons
        bool is_running = job.running.load();
        if (!is_running) {
            if (ImGui::Button("Render", ImVec2(-1, 0))) {
                // Start render job
                job.cancel.store(false);
                job.running.store(true);
                job.done.store(0);
                job.total.store(1);
                job.status_msg.clear();

                SceneConfig render_cfg = cfg;
                if (quick_render) {
                    render_cfg.viewport.pixel_width  /= 4;
                    render_cfg.viewport.pixel_height /= 4;
                    render_cfg.aa_samples = 1;
                }
                render_cfg.viewport.pixel_width  = std::max(1, render_cfg.viewport.pixel_width);
                render_cfg.viewport.pixel_height = std::max(1, render_cfg.viewport.pixel_height);

                if (job.thread.joinable()) job.thread.join();
                job.thread = std::thread([&job, render_cfg, &tex_pending]() mutable {
                    auto t0 = std::chrono::steady_clock::now();
                    try {
                        auto algo = render_cfg.create_algorithm();
                        auto buf  = std::make_unique<PixelBuffer>(
                            render_cfg.viewport.pixel_width,
                            render_cfg.viewport.pixel_height,
                            render_cfg.bit_depth == 16
                                ? PixelBuffer::Depth::U16
                                : PixelBuffer::Depth::U8);

                        TileRenderer renderer;
                        renderer.thread_count  = render_cfg.thread_count;
                        renderer.tile_size     = render_cfg.tile_size;
                        renderer.aa_samples    = render_cfg.aa_samples;
                        renderer.show_progress = false;
                        renderer.cancel_flag   = &job.cancel;
                        renderer.progress_cb   = [&job](int d, int t) {
                            job.done.store(d);
                            job.total.store(t);
                        };

                        renderer.render(*algo, *buf, render_cfg.viewport);
                        PostProcessor::apply(*buf, render_cfg.postprocess);

                        double elapsed = std::chrono::duration<double>(
                            std::chrono::steady_clock::now() - t0).count();

                        {
                            std::lock_guard<std::mutex> lk(job.result_mtx);
                            job.status_msg =
                                "Render: " + std::to_string(elapsed).substr(0, 4) + "s";
                            job.result = std::move(buf);
                        }
                    } catch (const std::exception& e) {
                        std::lock_guard<std::mutex> lk(job.result_mtx);
                        job.status_msg = std::string("Error: ") + e.what();
                    }
                    job.running.store(false);
                    tex_pending.store(true);
                });
                status_msg = "Rendering...";
            }
        } else {
            // Progress bar + cancel
            int d = job.done.load(), t = job.total.load();
            float frac = (t > 0) ? static_cast<float>(d) / t : 0.f;
            ImGui::ProgressBar(frac, ImVec2(-60, 0));
            ImGui::SameLine();
            if (ImGui::Button("Cancel")) {
                job.cancel.store(true);
                status_msg = "Cancelling...";
            }
        }

        // Save output button
        if (preview_tex && !is_running) {
            ImGui::Separator();
            if (ImGui::Button("Save image...", ImVec2(-1, 0))) {
                const char* filters[] = {
                    "PNG Files\0*.png\0",
                    "TIFF Files\0*.tiff\0",
                    "EXR Files\0*.exr\0"
                };
                const char* exts[] = {"png", "tiff", "exr"};
                std::string p = save_file_dialog(filters[fmt_idx], exts[fmt_idx]);
                if (!p.empty()) {
                    std::lock_guard<std::mutex> lk(job.result_mtx);
                    if (job.result) {
                        try {
                            if (fmt_idx == 1)
                                TiffWriter::write(*job.result, p, cfg.output_dpi);
                            else if (fmt_idx == 2)
                                ExrWriter::write(*job.result, p);
                            else
                                PngWriter::write(*job.result, p);
                            // Also save a full-res render stem to ImageSettings
                            auto stem = make_output_stem(cfg.algorithm_name);
                            SceneConfig saved_cfg = cfg;
                            saved_cfg.output_path = p;
                            save_settings_json(saved_cfg, stem);
                            status_msg = "Saved: " + p;
                        } catch (const std::exception& e) {
                            status_msg = std::string("Save error: ") + e.what();
                        }
                    }
                }
            }
        }

        // Quality warning
        if (!quality_warn.empty()) {
            ImGui::Separator();
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.7f, 0, 1));
            ImGui::TextWrapped("Warning: %s", quality_warn.c_str());
            ImGui::PopStyleColor();
        }

        // Status bar
        ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 30);
        ImGui::Separator();
        ImGui::TextUnformatted(status_msg.c_str());

        ImGui::End(); // Controls

        // ── Preview panel ────────────────────────────────────────────────────
        float panel_x  = 380.f;
        float panel_w  = static_cast<float>(ww) - panel_x;
        ImGui::SetNextWindowPos(ImVec2(panel_x, 0));
        ImGui::SetNextWindowSize(ImVec2(panel_w, static_cast<float>(wh)));
        ImGui::Begin("Preview", nullptr,
                     ImGuiWindowFlags_NoMove     | ImGuiWindowFlags_NoResize  |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

        if (preview_tex) {
            int tw, th;
            SDL_QueryTexture(preview_tex, nullptr, nullptr, &tw, &th);
            float avail_w = ImGui::GetContentRegionAvail().x;
            float avail_h = ImGui::GetContentRegionAvail().y;
            float scale   = std::min(avail_w / tw, avail_h / th) * zoom;
            float draw_w  = tw * scale;
            float draw_h  = th * scale;

            ImVec2 cursor = ImGui::GetCursorScreenPos();
            float img_x   = cursor.x + (avail_w - draw_w) * 0.5f + pan_x;
            float img_y   = cursor.y + (avail_h - draw_h) * 0.5f + pan_y;

            ImDrawList* dl = ImGui::GetWindowDrawList();
            dl->AddImage(reinterpret_cast<ImTextureID>(preview_tex),
                         ImVec2(img_x, img_y),
                         ImVec2(img_x + draw_w, img_y + draw_h));

            // Pan on middle mouse / drag
            if (ImGui::IsWindowHovered()) {
                if (io.MouseDown[1] || io.MouseDown[2]) {
                    pan_x += io.MouseDelta.x;
                    pan_y += io.MouseDelta.y;
                }
                // Scroll to zoom
                float wheel = io.MouseWheel;
                if (wheel != 0) {
                    zoom *= (1.0f + wheel * 0.1f);
                    zoom  = std::clamp(zoom, 0.05f, 50.f);
                }
                // Double-click: reset pan/zoom
                if (ImGui::IsMouseDoubleClicked(0)) {
                    pan_x = 0; pan_y = 0; zoom = 1;
                }
            }
        } else {
            ImGui::Text("No preview. Click Render to generate.");
        }

        ImGui::End(); // Preview
        } // mode == AppMode::GenerativeArt

        ImGui::Render();
        SDL_SetRenderDrawColor(ren, 30, 30, 30, 255);
        SDL_RenderClear(ren);
        ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData(), ren);
        SDL_RenderPresent(ren);
    }

    // Shutdown
    job.cancel.store(true);
    if (job.thread.joinable()) job.thread.join();

    if (preview_tex) SDL_DestroyTexture(preview_tex);
    if (pal_tex)     SDL_DestroyTexture(pal_tex);

    ImGui_ImplSDLRenderer2_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

} // namespace artgen::gui

#ifdef ARTGEN_WITH_SDL2

// Must be defined before SDL.h to suppress SDL's main() macro redefinition.
#define SDL_MAIN_HANDLED

#include "artgen/Preview.h"
#include "artgen/Renderer.h"
#include "artgen/Color.h"
#include "artgen/output/PngWriter.h"
#include <SDL.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace artgen {

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::vector<uint8_t> to_rgba8(const PixelBuffer& buf) {
    int W = buf.width(), H = buf.height();
    std::vector<uint8_t> px(W * H * 4);
    for (int y = 0; y < H; ++y)
        for (int x = 0; x < W; ++x) {
            RGBA c = buf.get(x, y);
            int i = (y * W + x) * 4;
            px[i+0] = to_u8(c.r);
            px[i+1] = to_u8(c.g);
            px[i+2] = to_u8(c.b);
            px[i+3] = to_u8(c.a);
        }
    return px;
}

static void upload_frame(SDL_Texture* tex, const PixelBuffer& buf) {
    auto px = to_rgba8(buf);
    SDL_UpdateTexture(tex, nullptr, px.data(), buf.width() * 4);
}

// Rerender the image at the current viewport (low AA, fast).
static PixelBuffer fast_render(IAlgorithm& algo, const Viewport& vp, int threads) {
    PixelBuffer buf(vp.pixel_width, vp.pixel_height);
    TileRenderer r;
    r.thread_count  = threads;
    r.tile_size     = 64;
    r.aa_samples    = 1;
    r.show_progress = false;
    r.render(algo, buf, vp);
    return buf;
}

// ── Preview window ────────────────────────────────────────────────────────────

void run_preview(IAlgorithm& algo, Viewport vp,
                 const PixelBuffer& initial_frame,
                 const PreviewOptions& opts)
{
    const int IMG_W = initial_frame.width();
    const int IMG_H = initial_frame.height();

    // Window size: fit image inside max dimensions
    int win_w = IMG_W, win_h = IMG_H;
    if (opts.max_window_w > 0 && win_w > opts.max_window_w) {
        win_h = win_h * opts.max_window_w / win_w;
        win_w = opts.max_window_w;
    }
    if (opts.max_window_h > 0 && win_h > opts.max_window_h) {
        win_w = win_w * opts.max_window_h / win_h;
        win_h = opts.max_window_h;
    }
    win_w = std::max(win_w, 64);
    win_h = std::max(win_h, 64);

    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return;
    }

    SDL_Window* window = SDL_CreateWindow(
        "artgen preview  |  drag=pan  scroll=zoom  ←/→=palette  R=render  S=save  Q=quit",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        win_w, win_h,
        SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
        SDL_Quit();
        return;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(
        window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
    }

    // RGBA32 = R,G,B,A bytes in memory on little-endian (what we produce)
    SDL_Texture* tex = SDL_CreateTexture(renderer,
        SDL_PIXELFORMAT_RGBA32,
        SDL_TEXTUREACCESS_STREAMING,
        IMG_W, IMG_H);

    PixelBuffer cur_frame = initial_frame;
    upload_frame(tex, cur_frame);

    float palette_phase  = 0.0f;
    bool  dragging       = false;
    int   drag_start_x   = 0, drag_start_y = 0;
    Viewport drag_vp     = vp;
    bool  dirty          = false;

    std::printf("  Preview: %dx%d window | %dx%d image\n", win_w, win_h, IMG_W, IMG_H);

    auto do_rerender = [&]() {
        // Viewport may have been scaled if window was resized; keep complex range,
        // but image pixels stay at IMG_W × IMG_H
        Viewport rv = vp;
        rv.pixel_width  = IMG_W;
        rv.pixel_height = IMG_H;
        cur_frame = fast_render(algo, rv, opts.fast_threads);
        upload_frame(tex, cur_frame);
        dirty = false;
        std::printf("  Re-rendered at viewport [%.4g,%.4g] × [%.4g,%.4g]\n",
                    rv.real_min, rv.real_max, rv.imag_min, rv.imag_max);
    };

    bool running = true;
    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            switch (ev.type) {
            case SDL_QUIT:
                running = false;
                break;

            case SDL_KEYDOWN:
                switch (ev.key.keysym.sym) {
                case SDLK_q: case SDLK_ESCAPE:
                    running = false;
                    break;

                case SDLK_r:
                    do_rerender();
                    break;

                case SDLK_s:
                    PngWriter::write(cur_frame, "preview_save.png");
                    std::printf("  Saved: preview_save.png\n");
                    break;

                case SDLK_LEFT:
                    palette_phase = std::fmod(palette_phase - 0.05f + 1.0f, 1.0f);
                    algo.set_palette_phase(palette_phase);
                    do_rerender();
                    break;

                case SDLK_RIGHT:
                    palette_phase = std::fmod(palette_phase + 0.05f, 1.0f);
                    algo.set_palette_phase(palette_phase);
                    do_rerender();
                    break;
                }
                break;

            case SDL_MOUSEBUTTONDOWN:
                if (ev.button.button == SDL_BUTTON_LEFT) {
                    dragging     = true;
                    drag_start_x = ev.button.x;
                    drag_start_y = ev.button.y;
                    drag_vp      = vp;
                }
                break;

            case SDL_MOUSEBUTTONUP:
                if (ev.button.button == SDL_BUTTON_LEFT && dragging) {
                    dragging = false;
                    if (dirty) do_rerender();
                }
                break;

            case SDL_MOUSEMOTION:
                if (dragging) {
                    int cur_win_w, cur_win_h;
                    SDL_GetWindowSize(window, &cur_win_w, &cur_win_h);
                    double real_span = drag_vp.real_max - drag_vp.real_min;
                    double imag_span = drag_vp.imag_max - drag_vp.imag_min;
                    double dreal = -(ev.motion.x - drag_start_x) * real_span / cur_win_w;
                    double dimag = -(ev.motion.y - drag_start_y) * imag_span / cur_win_h;
                    vp.real_min = drag_vp.real_min + dreal;
                    vp.real_max = drag_vp.real_max + dreal;
                    vp.imag_min = drag_vp.imag_min + dimag;
                    vp.imag_max = drag_vp.imag_max + dimag;
                    dirty = true;
                }
                break;

            case SDL_MOUSEWHEEL: {
                int mx, my;
                SDL_GetMouseState(&mx, &my);
                int cur_win_w, cur_win_h;
                SDL_GetWindowSize(window, &cur_win_w, &cur_win_h);

                double factor = (ev.wheel.y > 0) ? 0.85 : 1.0 / 0.85;
                double cx = vp.real_min + (vp.real_max - vp.real_min) * mx / cur_win_w;
                double cy = vp.imag_min + (vp.imag_max - vp.imag_min) * my / cur_win_h;
                double rw = (vp.real_max - vp.real_min) * factor;
                double ih = (vp.imag_max - vp.imag_min) * factor;
                double fx = (double)mx / cur_win_w;
                double fy = (double)my / cur_win_h;
                vp.real_min = cx - fx * rw;
                vp.real_max = vp.real_min + rw;
                vp.imag_min = cy - fy * ih;
                vp.imag_max = vp.imag_min + ih;
                do_rerender();
                break;
            }

            case SDL_WINDOWEVENT:
                if (ev.window.event == SDL_WINDOWEVENT_RESIZED)
                    do_rerender();
                break;
            }
        }

        // Render to window (stretch texture to fill window)
        int cur_win_w, cur_win_h;
        SDL_GetWindowSize(window, &cur_win_w, &cur_win_h);
        SDL_Rect dst{0, 0, cur_win_w, cur_win_h};
        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, tex, nullptr, &dst);
        SDL_RenderPresent(renderer);
        SDL_Delay(16); // ~60 fps
    }

    SDL_DestroyTexture(tex);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

} // namespace artgen

#endif // ARTGEN_WITH_SDL2

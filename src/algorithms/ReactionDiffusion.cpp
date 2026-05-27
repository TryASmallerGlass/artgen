#include "artgen/algorithms/ReactionDiffusion.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <vector>

namespace artgen {

ReactionDiffusion::ReactionDiffusion() {
    palette = Palette::ice();
}

void ReactionDiffusion::set_preset(const std::string& n) {
    if (n == "mitosis")     { feed = 0.0367f; kill = 0.0649f; }
    else if (n == "worms")  { feed = 0.082f;  kill = 0.061f;  }
    else if (n == "maze")   { feed = 0.029f;  kill = 0.057f;  }
    else if (n == "spots")  { feed = 0.037f;  kill = 0.061f;  }
    else if (n == "fingerprint") { feed = 0.037f; kill = 0.065f; }
    // default == coral: feed=0.0545, kill=0.062 (already set)
}

void ReactionDiffusion::render(PixelBuffer& buf, const Viewport& vp) const {
    const int W = vp.pixel_width;
    const int H = vp.pixel_height;
    const int N = W * H;

    // Double-buffered U and V grids
    std::vector<float> U(N, 1.0f), V(N, 0.0f);
    std::vector<float> nU(N), nV(N);

    // Seed random V patches using an LCG
    uint32_t rng = seed;
    auto lcg = [&]() -> uint32_t { return rng = rng * 1664525u + 1013904223u; };

    // Ensure a minimum density of seeds — at least 1 per 400 cells
    int n_seeds = std::max(3, N / 400);
    for (int i = 0; i < n_seeds; ++i) {
        int cx = static_cast<int>(lcg() % static_cast<uint32_t>(W));
        int cy = static_cast<int>(lcg() % static_cast<uint32_t>(H));
        int r  = 2 + static_cast<int>(lcg() % 4u);
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                int px = (cx + dx + W) % W;
                int py = (cy + dy + H) % H;
                if (dx*dx + dy*dy <= r*r) {
                    U[py*W + px] = 0.5f;
                    V[py*W + px] = 0.25f + (lcg() & 0xFF) / 2048.0f;
                }
            }
        }
    }

    // Simulation loop
    const float f  = feed, k  = kill;
    const float du = Du * dt, dv = Dv * dt;

    for (int step = 0; step < steps; ++step) {
        for (int y = 0; y < H; ++y) {
            int yn = (y > 0)     ? y-1 : H-1;
            int ys = (y < H-1)   ? y+1 : 0;
            for (int x = 0; x < W; ++x) {
                int xw = (x > 0)   ? x-1 : W-1;
                int xe = (x < W-1) ? x+1 : 0;

                int c  = y*W + x;
                int n_ = yn*W + x;
                int s_ = ys*W + x;
                int w_ = y*W + xw;
                int e_ = y*W + xe;

                // 5-point Laplacian
                float lapU = U[n_] + U[s_] + U[w_] + U[e_] - 4.0f*U[c];
                float lapV = V[n_] + V[s_] + V[w_] + V[e_] - 4.0f*V[c];

                float u = U[c], v = V[c];
                float uvv = u * v * v;

                nU[c] = std::clamp(u + du*lapU - uvv + f*(1.0f - u), 0.0f, 1.0f);
                nV[c] = std::clamp(v + dv*lapV + uvv - (f + k)*v,    0.0f, 1.0f);
            }
        }
        std::swap(U, nU);
        std::swap(V, nV);
    }

    // Color by V concentration through palette
    float vmax = *std::max_element(V.begin(), V.end());
    float vmin = *std::min_element(V.begin(), V.end());
    float vrange = vmax - vmin;

    for (int py = 0; py < H; ++py)
        for (int px = 0; px < W; ++px) {
            float t = (vrange > 1e-6f)
                ? (V[py*W + px] - vmin) / vrange
                : 0.0f;
            buf.set(px, py, palette.sample(t));
        }
}

} // namespace artgen

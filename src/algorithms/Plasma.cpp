#include "artgen/algorithms/Plasma.h"
#include <algorithm>
#include <cmath>

namespace artgen {

PlasmaAlgorithm::PlasmaAlgorithm() {
    palette = Palette::classic_mandelbrot();
}

// ── Bilinear sampler ──────────────────────────────────────────────────────────

float PlasmaAlgorithm::bilinear(const std::vector<float>& grid, int gsize,
                                 float gx, float gy) {
    int x0 = static_cast<int>(gx);
    int y0 = static_cast<int>(gy);
    int x1 = std::min(x0 + 1, gsize - 1);
    int y1 = std::min(y0 + 1, gsize - 1);
    float fx = gx - static_cast<float>(x0);
    float fy = gy - static_cast<float>(y0);

    auto g = [&](int x, int y) -> float {
        return grid[static_cast<size_t>(y) * gsize + x];
    };
    return (1.0f - fx) * (1.0f - fy) * g(x0, y0)
         +          fx  * (1.0f - fy) * g(x1, y0)
         + (1.0f - fx) *          fy  * g(x0, y1)
         +          fx  *          fy  * g(x1, y1);
}

// ── Diamond-Square generator ──────────────────────────────────────────────────

void PlasmaAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    int W = vp.pixel_width, H = vp.pixel_height;
    int oct  = std::max(1, std::min(octaves, 12));
    int n    = 1 << oct;          // 2^octaves
    int gsize = n + 1;            // e.g. octaves=8 → 257

    std::vector<float> grid(static_cast<size_t>(gsize) * gsize, 0.0f);

    // Seeded xorshift32 RNG (seed=0 is seeded as a non-zero constant)
    uint32_t rng_state = (seed == 0) ? 2463534242u : seed;
    auto rng = [&]() -> float {
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        return static_cast<float>(rng_state & 0x7FFFFFFFu) / 2147483647.0f;
    };
    auto rand11 = [&]() -> float { return rng() * 2.0f - 1.0f; };

    auto idx = [&](int x, int y) -> float& {
        return grid[static_cast<size_t>(y) * gsize + x];
    };

    // Initialise four corners
    idx(0, 0) = rand11();
    idx(n, 0) = rand11();
    idx(0, n) = rand11();
    idx(n, n) = rand11();

    float scale = 1.0f;

    for (int step = n; step > 1; step /= 2) {
        int half = step / 2;
        scale *= roughness;

        // Diamond step: centre of each square
        for (int y = 0; y < n; y += step) {
            for (int x = 0; x < n; x += step) {
                float avg = (idx(x,      y     ) + idx(x+step, y     ) +
                             idx(x,      y+step) + idx(x+step, y+step)) * 0.25f;
                idx(x + half, y + half) = avg + rand11() * scale;
            }
        }

        // Square step: midpoints of each edge
        for (int y = 0; y <= n; y += half) {
            // Every other row is offset by half so we skip diamond centres
            int x_off = ((y / half) % 2 == 0) ? half : 0;
            for (int x = x_off; x <= n; x += step) {
                float sum = 0.0f;
                int   cnt = 0;
                if (x - half >= 0) { sum += idx(x - half, y); ++cnt; }
                if (x + half <= n) { sum += idx(x + half, y); ++cnt; }
                if (y - half >= 0) { sum += idx(x, y - half); ++cnt; }
                if (y + half <= n) { sum += idx(x, y + half); ++cnt; }
                idx(x, y) = sum / static_cast<float>(cnt) + rand11() * scale;
            }
        }
    }

    // Normalise to [0, 1]
    float mn = *std::min_element(grid.begin(), grid.end());
    float mx = *std::max_element(grid.begin(), grid.end());
    float range = (mx - mn > 1e-6f) ? (mx - mn) : 1.0f;

    // Sample grid for each pixel and map through palette
    float inv_w = (W > 1) ? static_cast<float>(gsize - 1) / (W - 1) : 0.0f;
    float inv_h = (H > 1) ? static_cast<float>(gsize - 1) / (H - 1) : 0.0f;

    for (int py = 0; py < H; ++py) {
        float gy = static_cast<float>(py) * inv_h;
        for (int px = 0; px < W; ++px) {
            float gx = static_cast<float>(px) * inv_w;
            float v  = bilinear(grid, gsize, gx, gy);
            float t  = (v - mn) / range;
            buf.set(px, py, palette.sample(t));
        }
    }
}

} // namespace artgen

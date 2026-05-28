#include "artgen/algorithms/Attractor.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace artgen {

AttractorAlgorithm::AttractorAlgorithm() {
    palette = Palette::fire();
}

// ── Iteration step ─────────────────────────────────────────────────────────────

void AttractorAlgorithm::advance(double& x, double& y) const {
    double nx, ny;
    switch (type) {
    case AttractorType::DeJong:
        nx = std::sin(a * y) - std::cos(b * x);
        ny = std::sin(c * x) - std::cos(d * y);
        break;
    case AttractorType::Clifford:
    default:
        nx = std::sin(a * y) + c * std::cos(a * x);
        ny = std::sin(b * x) + d * std::cos(b * y);
        break;
    }
    x = nx;
    y = ny;
}

// ── Main render ───────────────────────────────────────────────────────────────

void AttractorAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    int W = vp.pixel_width, H = vp.pixel_height;

    std::vector<uint32_t> density(static_cast<size_t>(W) * H, 0);
    uint32_t max_density = 0;

    double x = 0.5, y = 0.0;

    // Warm-up: discard initial transient so the orbit settles onto the attractor
    for (int i = 0; i < warmup; ++i)
        advance(x, y);

    double real_range = vp.real_max - vp.real_min;
    double imag_range = vp.imag_max - vp.imag_min;

    // Accumulate density histogram
    for (int i = 0; i < iterations; ++i) {
        advance(x, y);

        int px = static_cast<int>((x - vp.real_min) / real_range * (W - 1) + 0.5);
        int py = static_cast<int>((y - vp.imag_min) / imag_range * (H - 1) + 0.5);

        if (px >= 0 && px < W && py >= 0 && py < H) {
            uint32_t& dens = density[static_cast<size_t>(py) * W + px];
            ++dens;
            if (dens > max_density) max_density = dens;
        }
    }

    if (max_density == 0) {
        buf.fill({0.0f, 0.0f, 0.0f, 1.0f});
        return;
    }

    // Map log-normalised density through palette
    double log_max = std::log(1.0 + max_density);
    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            uint32_t dens = density[static_cast<size_t>(py) * W + px];
            float t = (dens == 0) ? 0.0f
                : static_cast<float>(std::log(1.0 + dens) / log_max);
            buf.set(px, py, palette.sample(t));
        }
    }
}

} // namespace artgen

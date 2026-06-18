#include "artgen/algorithms/Voronoi.h"
#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>
#include <vector>

namespace artgen {

VoronoiAlgorithm::VoronoiAlgorithm() {
    palette = Palette::electric();
}

void VoronoiAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    int W = vp.pixel_width, H = vp.pixel_height;
    int N = std::max(1, num_seeds);

    // Generate seed points in normalised [0,1]×[0,1] space
    std::vector<std::pair<float,float>> seeds(static_cast<size_t>(N));
    uint32_t rng_state = (seed == 0) ? 2463534242u : seed;
    auto rng = [&]() -> float {
        rng_state ^= rng_state << 13;
        rng_state ^= rng_state >> 17;
        rng_state ^= rng_state << 5;
        return static_cast<float>(rng_state & 0x7FFFFFFFu) / 2147483647.0f;
    };
    for (auto& s : seeds) { s.first = rng(); s.second = rng(); }

    // Distance function
    auto dist_fn = [&](float x1, float y1, float x2, float y2) -> float {
        float dx = x1 - x2, dy = y1 - y2;
        switch (metric) {
        case VoronoiMetric::Manhattan:  return std::abs(dx) + std::abs(dy);
        case VoronoiMetric::Chebyshev:  return std::max(std::abs(dx), std::abs(dy));
        default:                        return std::sqrt(dx*dx + dy*dy);
        }
    };

    float inv_w = (W > 1) ? 1.0f / (W - 1) : 0.0f;
    float inv_h = (H > 1) ? 1.0f / (H - 1) : 0.0f;

    for (int py = 0; py < H; ++py) {
        float ny = static_cast<float>(py) * inv_h;
        for (int px = 0; px < W; ++px) {
            float nx = static_cast<float>(px) * inv_w;

            float d1      = std::numeric_limits<float>::max();
            float d2      = std::numeric_limits<float>::max();
            int   nearest = 0;

            for (int i = 0; i < N; ++i) {
                float d = dist_fn(nx, ny, seeds[i].first, seeds[i].second);
                if (d < d1) { d2 = d1; d1 = d; nearest = i; }
                else if (d < d2) { d2 = d; }
            }

            float t = 0.0f;
            switch (mode) {
            case VoronoiMode::Cells: {
                // Hash seed index → pseudo-random palette position
                uint32_t h = static_cast<uint32_t>(nearest) * 2654435761u;
                h ^= h >> 16;
                t = static_cast<float>(h & 0xFFFFu) / 65535.0f;
                break;
            }
            case VoronoiMode::Distance: {
                // Euclidean max in [0,1]^2 is sqrt(2); Manhattan/Chebyshev max is 2/1
                float max_d = (metric == VoronoiMetric::Euclidean) ? 1.4143f
                            : (metric == VoronoiMetric::Manhattan)  ? 2.0f
                            :                                          1.0f;
                t = std::min(1.0f, d1 / max_d);
                break;
            }
            case VoronoiMode::Edge: {
                // Bright at Voronoi boundaries (where d1 ≈ d2)
                float sum = d1 + d2;
                float edge_t = (sum > 1e-6f) ? (d2 - d1) / sum : 1.0f;
                // Remap: 0 at border → 1 at border (invert and sharpen)
                t = 1.0f - std::min(1.0f, edge_t * 4.0f);
                break;
            }
            }

            buf.set(px, py, palette.sample(t));
        }
    }
}

} // namespace artgen

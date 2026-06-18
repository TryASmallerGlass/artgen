#include "artgen/algorithms/EscapeTime.h"
#include <algorithm>
#include <cmath>
#include <vector>

namespace artgen {

float EscapeTimeAlgorithm::compute_t(const IterResult& r) const {
    if (r.iter == max_iterations) return 0.0f;
    if (!smooth_coloring)
        return static_cast<float>(std::fmod(r.iter / color_cycle, 1.0));

    double log_er   = std::log(escape_radius);
    double log_base = std::log(std::max(power, 1.0001)); // generalised: log_2 for power==2
    double mod      = std::sqrt(r.zr * r.zr + r.zi * r.zi);
    double nu       = std::log(std::log(mod) / log_er) / log_base;
    // +1 offset (Linas Vepstas formula) keeps smooth_iter positive even for fast escapes
    double si     = static_cast<double>(r.iter) + 1.0 - nu;
    return static_cast<float>(std::fmod(std::max(0.0, si) / color_cycle, 1.0));
}

void EscapeTimeAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    int W = vp.pixel_width, H = vp.pixel_height;

    if (coloring_mode == ColoringMode::HistogramEq) {
        // Two-pass: collect smooth_t values, build CDF, remap.
        std::vector<float> tvals(static_cast<size_t>(W) * H);

        for (int py = 0; py < H; ++py)
            for (int px = 0; px < W; ++px)
                tvals[py * W + px] = compute_t(iterate(vp.pixel_to_real(px),
                                                       vp.pixel_to_imag(py)));

        // Sort non-interior values for CDF
        std::vector<float> sorted;
        sorted.reserve(tvals.size());
        for (float v : tvals)
            if (v > 0.0f) sorted.push_back(v);
        std::sort(sorted.begin(), sorted.end());
        int N = static_cast<int>(sorted.size());

        for (int py = 0; py < H; ++py) {
            for (int px = 0; px < W; ++px) {
                float v = tvals[py * W + px];
                float t = 0.0f;
                if (v > 0.0f && N > 0) {
                    auto it = std::lower_bound(sorted.begin(), sorted.end(), v);
                    t = static_cast<float>(std::distance(sorted.begin(), it)) / N;
                }
                buf.set(px, py, palette.sample(t));
            }
        }
        return;
    }

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            IterResult r = iterate(vp.pixel_to_real(px), vp.pixel_to_imag(py));

            RGBA c;
            if (coloring_mode == ColoringMode::OrbitTrap) {
                float t = (r.iter == max_iterations) ? 0.0f
                    : static_cast<float>(std::fmod(r.min_trap / (escape_radius / color_cycle), 1.0));
                c = palette.sample(t);
            } else {
                c = palette.sample(compute_t(r));
            }
            buf.set(px, py, c);
        }
    }
}

} // namespace artgen

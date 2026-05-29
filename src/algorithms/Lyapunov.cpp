#include "artgen/algorithms/Lyapunov.h"
#include <algorithm>
#include <cmath>

namespace artgen {

void LyapunovAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    const int W = vp.pixel_width;
    const int H = vp.pixel_height;
    const int seq_len = static_cast<int>(sequence.size());

    // Normalisation divisor: a comfortable maximum |λ| for the stable range.
    // Values more negative than -norm_div are clamped to t=1.
    constexpr double norm_div = 3.0;

    for (int py = 0; py < H; ++py) {
        for (int px = 0; px < W; ++px) {
            const double a = vp.pixel_to_real(px);
            const double b = vp.pixel_to_imag(py);

            // Run warm-up iterations to settle the logistic map
            double x = seed_x;
            for (int n = 0; n < warmup; ++n) {
                const double r = (sequence[n % seq_len] == 'B') ? b : a;
                x = r * x * (1.0 - x);
            }

            // Accumulate Lyapunov exponent
            double lambda = 0.0;
            for (int n = 0; n < iterations; ++n) {
                const double r = (sequence[n % seq_len] == 'B') ? b : a;
                x = r * x * (1.0 - x);
                // Derivative of logistic map w.r.t. x: |r*(1 - 2x)|
                const double deriv = std::abs(r * (1.0 - 2.0 * x));
                if (deriv > 0.0)
                    lambda += std::log(deriv);
            }
            lambda /= iterations;

            // Map to palette:
            //   λ <  0  (stable)  → t = clamp(-λ / norm_div, 0, 1)  → colorful
            //   λ >= 0  (chaotic) → t = 0  → dark end of palette
            float t = 0.0f;
            if (lambda < 0.0)
                t = static_cast<float>(std::min(-lambda / norm_div, 1.0));

            buf.set(px, py, palette.sample(t));
        }
    }
}

} // namespace artgen

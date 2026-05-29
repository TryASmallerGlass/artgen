#include "artgen/algorithms/Nova.h"
#include "artgen/Color.h"
#include <complex>
#include <cmath>

namespace artgen {

void NovaAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    using cx = std::complex<double>;
    const double two_pi = 2.0 * 3.14159265358979323846;
    const int    p      = power;

    // Fixed c for Julia-mode; pixel-driven c for Mandelbrot-mode
    const cx c_seed(seed_r, seed_i);

    for (int py = 0; py < vp.pixel_height; ++py) {
        for (int px = 0; px < vp.pixel_width; ++px) {
            const cx pixel(vp.pixel_to_real(px), vp.pixel_to_imag(py));

            cx z = julia_mode ? pixel : cx(1.0, 0.0);
            const cx c = julia_mode ? c_seed : pixel;

            bool converged = false;
            bool escaped   = false;
            int  iter      = 0;

            for (; iter < max_iterations; ++iter) {
                // Check escape first (some orbits diverge rather than converge)
                if (std::abs(z) > escape_radius) {
                    escaped = true;
                    break;
                }

                // Newton step:  z -= R * (z^p - 1) / (p * z^{p-1})  + c
                const cx z_prev = z;
                const cx zp_minus1 = std::pow(z, p) - cx(1.0);
                const cx denom     = cx(static_cast<double>(p)) * std::pow(z, p - 1);
                if (std::abs(denom) < 1e-12) break;

                z -= relaxation * zp_minus1 / denom;
                z += c;

                // Orbital convergence: the iteration has settled when z stops moving.
                // This catches fixed points of the full Nova map F(z) = z - R*f/f' + c,
                // not only roots of z^p = 1 (which the +c term would immediately disturb).
                if (std::abs(z - z_prev) < tolerance) {
                    converged = true;
                    break;
                }
            }

            RGBA color;
            if (escaped) {
                // Smooth escape-time coloring (same formula as EscapeTimeAlgorithm)
                double log_escape = std::log(escape_radius);
                double nu  = std::log(std::log(std::abs(z)) / log_escape) / std::log(static_cast<double>(p));
                double smooth = static_cast<double>(iter) + 1.0 - nu;
                float  t = static_cast<float>(std::fmod(smooth / 32.0, 1.0));
                if (t < 0.0f) t += 1.0f;
                color = palette.sample(t);
            } else if (converged) {
                // Newton basin coloring: hue encodes which root, brightness encodes speed
                double angle = std::arg(z);
                if (angle < 0.0) angle += two_pi;
                float hue        = static_cast<float>(angle / two_pi) * 360.0f;
                float brightness = 1.0f - static_cast<float>(iter) / max_iterations;
                brightness       = 0.2f + brightness * 0.8f;
                RGB rgb = hsv_to_rgb({hue, saturation, brightness});
                color = {rgb.r, rgb.g, rgb.b, 1.0f};
            } else {
                // Neither converged nor escaped — dark background
                color = palette.sample(0.0f);
            }

            buf.set(px, py, color);
        }
    }
}

} // namespace artgen

#include "artgen/algorithms/Newton.h"
#include "artgen/Color.h"
#include <complex>
#include <cmath>

namespace artgen {

void NewtonAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    using cx = std::complex<double>;
    const double two_pi = 2.0 * 3.14159265358979323846;
    const int    p      = power;

    for (int py = 0; py < vp.pixel_height; ++py) {
        for (int px = 0; px < vp.pixel_width; ++px) {
            cx z(vp.pixel_to_real(px), vp.pixel_to_imag(py));

            int iter = 0;
            for (; iter < max_iterations; ++iter) {
                cx zp  = std::pow(z, p - 1);
                cx denom = cx(p) * zp;
                if (std::abs(denom) < 1e-12) break;
                z -= (z * zp - cx(1.0)) / denom; // z - (z^p - 1)/(p*z^{p-1})
                if (std::abs(std::pow(z, p) - cx(1.0)) < tolerance) break;
            }

            // Which root? angle in [0, 2π) → root index
            double angle = std::arg(z);
            if (angle < 0.0) angle += two_pi;
            float hue        = static_cast<float>(angle / two_pi) * 360.0f;
            float brightness = 1.0f - static_cast<float>(iter) / max_iterations;
            brightness       = 0.2f + brightness * 0.8f; // keep minimum brightness

            RGB rgb = hsv_to_rgb({hue, saturation, brightness});
            buf.set(px, py, {rgb.r, rgb.g, rgb.b, 1.0f});
        }
    }
}

} // namespace artgen

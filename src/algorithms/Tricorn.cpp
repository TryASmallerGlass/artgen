#include "artgen/algorithms/Tricorn.h"
#include <cmath>
#include <limits>

namespace artgen {

TricornAlgorithm::TricornAlgorithm() {
    palette = Palette::ice();
}

IterResult TricornAlgorithm::iterate(double cr, double ci) const {
    const double er2 = escape_radius * escape_radius;
    double zr = 0.0, zi = 0.0;
    double min_trap = std::numeric_limits<double>::max();
    int iter = 0;

    // conj(z)^2 + c:  flip sign of zi before squaring
    // conj(z)^2 = (zr - i*zi)^2 = zr^2 - zi^2 - 2i*zr*zi
    // Compared to z^2: real part is identical, imaginary part is negated.
    while (zr*zr + zi*zi < er2 && iter < max_iterations) {
        double tmp = zr*zr - zi*zi + cr;
        zi = -2.0 * zr * zi + ci;   // <-- negated vs. standard Mandelbrot
        zr = tmp;
        ++iter;
        double trap = std::sqrt(zr*zr + zi*zi);
        if (trap < min_trap) min_trap = trap;
    }
    return {iter, zr, zi, min_trap};
}

} // namespace artgen

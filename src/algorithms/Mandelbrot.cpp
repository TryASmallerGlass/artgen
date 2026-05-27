#include "artgen/algorithms/Mandelbrot.h"
#include <cmath>

namespace artgen {

MandelbrotAlgorithm::MandelbrotAlgorithm() {
    palette = Palette::classic_mandelbrot();
}

IterResult MandelbrotAlgorithm::iterate(double cr, double ci) const {
    const double er2 = escape_radius * escape_radius;
    double zr = 0.0, zi = 0.0;
    double min_trap = std::numeric_limits<double>::max();
    int iter = 0;

    while (zr*zr + zi*zi < er2 && iter < max_iterations) {
        double tmp = zr*zr - zi*zi + cr;
        zi = 2.0 * zr * zi + ci;
        zr = tmp;
        ++iter;
        double trap = std::sqrt(zr*zr + zi*zi);
        if (trap < min_trap) min_trap = trap;
    }
    return {iter, zr, zi, min_trap};
}

} // namespace artgen

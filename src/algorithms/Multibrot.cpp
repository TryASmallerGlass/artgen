#include "artgen/algorithms/Multibrot.h"
#include <cmath>
#include <limits>

namespace artgen {

MultibrotAlgorithm::MultibrotAlgorithm() {
    palette = Palette::classic_mandelbrot();
    power   = 3.0; // default: cubic Mandelbrot
}

IterResult MultibrotAlgorithm::iterate(double cr, double ci) const {
    const double er2 = escape_radius * escape_radius;
    double zr = 0.0, zi = 0.0;
    double min_trap = std::numeric_limits<double>::max();
    int iter = 0;

    while (zr*zr + zi*zi < er2 && iter < max_iterations) {
        // z = z^power + c  via polar form: z = r*exp(i*theta)
        // z^n = r^n * exp(i*n*theta)
        double r2    = zr*zr + zi*zi;
        double r     = std::sqrt(r2);
        double theta = std::atan2(zi, zr);
        double rn    = std::pow(r, power);
        double nt    = power * theta;
        zr = rn * std::cos(nt) + cr;
        zi = rn * std::sin(nt) + ci;
        ++iter;
        double trap = std::sqrt(zr*zr + zi*zi);
        if (trap < min_trap) min_trap = trap;
    }
    return {iter, zr, zi, min_trap};
}

} // namespace artgen

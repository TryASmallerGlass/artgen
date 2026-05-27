#include "artgen/algorithms/Julia.h"
#include <cmath>
#include <limits>

namespace artgen {

JuliaAlgorithm::JuliaAlgorithm() {
    palette = Palette::electric();
}

IterResult JuliaAlgorithm::iterate(double cr, double ci) const {
    const double er2 = escape_radius * escape_radius;
    // z_0 = pixel, c = fixed seed
    double zr = cr, zi = ci;
    double min_trap = std::numeric_limits<double>::max();
    int iter = 0;

    while (zr*zr + zi*zi < er2 && iter < max_iterations) {
        double tmp = zr*zr - zi*zi + seed_r;
        zi = 2.0 * zr * zi + seed_i;
        zr = tmp;
        ++iter;
        double trap = std::sqrt(zr*zr + zi*zi);
        if (trap < min_trap) min_trap = trap;
    }
    return {iter, zr, zi, min_trap};
}

} // namespace artgen

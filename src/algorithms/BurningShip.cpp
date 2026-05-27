#include "artgen/algorithms/BurningShip.h"
#include <cmath>
#include <limits>

namespace artgen {

BurningShipAlgorithm::BurningShipAlgorithm() {
    palette = Palette::fire();
    // Burning Ship is typically viewed upside-down
}

IterResult BurningShipAlgorithm::iterate(double cr, double ci) const {
    const double er2 = escape_radius * escape_radius;
    double zr = 0.0, zi = 0.0;
    double min_trap = std::numeric_limits<double>::max();
    int iter = 0;

    while (zr*zr + zi*zi < er2 && iter < max_iterations) {
        // Take absolute values before squaring — the defining feature
        double azr = std::abs(zr), azi = std::abs(zi);
        double tmp = azr*azr - azi*azi + cr;
        zi = 2.0 * azr * azi + ci;
        zr = tmp;
        ++iter;
        double trap = std::sqrt(zr*zr + zi*zi);
        if (trap < min_trap) min_trap = trap;
    }
    return {iter, zr, zi, min_trap};
}

} // namespace artgen

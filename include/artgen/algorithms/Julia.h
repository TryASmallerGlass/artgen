#pragma once
#include "artgen/algorithms/EscapeTime.h"

namespace artgen {

class JuliaAlgorithm : public EscapeTimeAlgorithm {
public:
    double seed_r = -0.7;
    double seed_i =  0.27015;

    JuliaAlgorithm();
    const char* name() const override { return "julia"; }

protected:
    // cr/ci are the pixel position (z_0); seed is the fixed c
    IterResult iterate(double cr, double ci) const override;
};

} // namespace artgen

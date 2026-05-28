#pragma once
#include "artgen/algorithms/EscapeTime.h"

namespace artgen {

// Generalised Mandelbrot: z_{n+1} = z^power + c
// power == 2 reproduces the Mandelbrot set exactly.
// Non-integer powers are computed via polar form (r^n, n*theta).
class MultibrotAlgorithm : public EscapeTimeAlgorithm {
public:
    MultibrotAlgorithm();
    const char* name() const override { return "multibrot"; }

protected:
    IterResult iterate(double cr, double ci) const override;
};

} // namespace artgen

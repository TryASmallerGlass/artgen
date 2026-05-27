#pragma once
#include "artgen/algorithms/EscapeTime.h"

namespace artgen {

class MandelbrotAlgorithm : public EscapeTimeAlgorithm {
public:
    MandelbrotAlgorithm();
    const char* name() const override { return "mandelbrot"; }

protected:
    IterResult iterate(double cr, double ci) const override;
};

} // namespace artgen

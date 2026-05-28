#pragma once
#include "artgen/algorithms/EscapeTime.h"

namespace artgen {

// Tricorn (Mandelbar): z_{n+1} = conj(z)^2 + c
// The conjugate negates the imaginary part before squaring, producing
// a distinctive three-lobed shape with jagged, spiky tips.
class TricornAlgorithm : public EscapeTimeAlgorithm {
public:
    TricornAlgorithm();
    const char* name() const override { return "tricorn"; }

protected:
    IterResult iterate(double cr, double ci) const override;
};

} // namespace artgen

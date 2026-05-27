#pragma once
#include "artgen/IAlgorithm.h"

namespace artgen {

// Newton fractal for p(z) = z^power - 1.
// Color = HSV where hue encodes which root converged,
// brightness encodes convergence speed.
class NewtonAlgorithm : public IAlgorithm {
public:
    int    power          = 3;
    int    max_iterations = 200;
    double tolerance      = 1e-6;
    float  saturation     = 0.8f;

    const char* name() const override { return "newton"; }
    void render(PixelBuffer& buf, const Viewport& vp) const override;
};

} // namespace artgen

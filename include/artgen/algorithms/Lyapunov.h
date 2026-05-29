#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"
#include <string>

namespace artgen {

// Lyapunov fractal.
//
// Each pixel maps to parameter pair (a, b) via the viewport.  A logistic map
//   x_{n+1} = r_n * x_n * (1 - x_n)
// is driven by an alternating sequence of parameters where 'A' selects a and
// 'B' selects b.  The Lyapunov exponent
//   λ = (1/N) * Σ ln|r_n * (1 - 2*x_n)|
// is then mapped through the palette: λ < 0 (stable) → bright colors,
// λ ≥ 0 (chaotic) → dark end of palette.
//
// Tileable — every pixel is computed independently.
class LyapunovAlgorithm : public IAlgorithm {
public:
    std::string sequence   = "AB";   // 'A' uses viewport real coord, 'B' uses imag
    int         warmup     = 200;    // logistic-map steps discarded before sampling
    int         iterations = 1000;   // steps used to compute λ
    double      seed_x     = 0.5;    // initial x₀ for the logistic map
    Palette     palette;

    const char* name()        const override { return "lyapunov"; }
    bool        is_tileable() const override { return true; }
    void render(PixelBuffer& buf, const Viewport& vp) const override;
    void set_palette_phase(float p) override { palette.phase = p; }
};

} // namespace artgen

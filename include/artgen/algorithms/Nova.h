#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"

namespace artgen {

// Nova fractal — relaxed Newton iteration with an additive injection of c:
//
//   z_{n+1} = z_n - R * (z_n^p - 1) / (p * z_n^{p-1}) + c
//
// Two modes:
//   Julia-type      (julia_mode=true):  z₀ = pixel,  c = fixed seed
//   Mandelbrot-type (julia_mode=false): z₀ = 1+0i,   c = pixel
//
// Coloring is layered:
//   - Converged pixels (|z^p - 1| < tolerance) → Newton basin HSV coloring:
//       hue from arg(z), brightness from iteration count
//   - Escaped pixels (|z| > escape_radius) → smooth escape-time via palette
//   - Non-converging, non-escaping pixels → palette at t=0
//
// Tileable — every pixel is independent.
class NovaAlgorithm : public IAlgorithm {
public:
    int    power          = 3;
    double relaxation     = 1.0;    // R — step-size multiplier (1.0 = standard Newton)
    bool   julia_mode     = false;  // false = Mandelbrot-type, true = Julia-type
    double seed_r         = 1.0;    // Julia-mode: fixed c (real part)
    double seed_i         = 0.0;    // Julia-mode: fixed c (imaginary part)
    int    max_iterations = 256;
    double tolerance      = 1e-6;   // convergence threshold: |z^p - 1|
    double escape_radius  = 1e6;    // escape threshold: |z|
    float  saturation     = 0.8f;   // HSV saturation for converged basins
    Palette palette;                // used for the escaped (exterior) region

    const char* name()        const override { return "nova"; }
    bool        is_tileable() const override { return true; }
    void render(PixelBuffer& buf, const Viewport& vp) const override;
    void set_palette_phase(float p) override { palette.phase = p; }
};

} // namespace artgen

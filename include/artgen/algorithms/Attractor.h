#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"
#include <cstdint>
#include <string>

namespace artgen {

enum class AttractorType { Clifford, DeJong, Ikeda };

// Strange attractor rendered as an orbit-density histogram.
//
// The orbit is iterated for `iterations` steps (after `warmup` discarded steps).
// Each visited pixel increments a density counter; the log-normalised density
// is mapped through the palette.
//
// Clifford:  x' = sin(a*y) + c*cos(a*x),  y' = sin(b*x) + d*cos(b*y)
// De Jong:   x' = sin(a*y) - cos(b*x),    y' = sin(c*x) - cos(d*y)
// Ikeda:     τ  = 0.4 - 6/(1+x²+y²)
//            x' = 1 + u*(x*cos τ - y*sin τ),  y' = u*(x*sin τ + y*cos τ)
//
// Not tileable — the full orbit must be accumulated in a single pass.
class AttractorAlgorithm : public IAlgorithm {
public:
    AttractorType type       = AttractorType::Clifford;
    double a = -1.4, b = 1.6, c = 1.0, d = 0.7;
    double u = 0.9;   // Ikeda: tuning parameter (chaotic for u ≈ 0.9)
    int    iterations = 8'000'000;
    int    warmup     = 1000;
    Palette palette;

    AttractorAlgorithm();
    const char* name()        const override { return "attractor"; }
    bool        is_tileable() const override { return false; }
    void render(PixelBuffer& buf, const Viewport& vp) const override;
    void set_palette_phase(float p) override { palette.phase = p; }

private:
    void advance(double& x, double& y) const;
};

} // namespace artgen

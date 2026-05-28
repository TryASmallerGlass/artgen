#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"
#include <cstdint>
#include <string>

namespace artgen {

enum class AttractorType { Clifford, DeJong };

// Strange attractor rendered as an orbit-density histogram.
//
// The orbit is iterated for `iterations` steps (after `warmup` discarded steps).
// Each visited pixel increments a density counter; the log-normalised density
// is mapped through the palette.
//
// Clifford:  x' = sin(a*y) + c*cos(a*x),  y' = sin(b*x) + d*cos(b*y)
// De Jong:   x' = sin(a*y) - cos(b*x),    y' = sin(c*x) - cos(d*y)
//
// Not tileable — the full orbit must be accumulated in a single pass.
class AttractorAlgorithm : public IAlgorithm {
public:
    AttractorType type       = AttractorType::Clifford;
    double a = -1.4, b = 1.6, c = 1.0, d = 0.7;
    int    iterations = 5'000'000;
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

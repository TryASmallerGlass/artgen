#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"

namespace artgen {

enum class ColoringMode { Smooth, HistogramEq, OrbitTrap };

struct IterResult {
    int    iter;
    double zr, zi;
    double min_trap; // min |z| seen after each step (for orbit trap)
};

class EscapeTimeAlgorithm : public IAlgorithm {
public:
    int          max_iterations  = 1000;
    double       escape_radius   = 2.0;
    double       color_cycle     = 64.0;
    double       power           = 2.0; // iteration exponent — used in smooth coloring formula
    bool         smooth_coloring = true;
    ColoringMode coloring_mode   = ColoringMode::Smooth;
    Palette      palette;

    void render(PixelBuffer& buf, const Viewport& vp) const final;
    void set_palette_phase(float p) override { palette.phase = p; }

protected:
    virtual IterResult iterate(double cr, double ci) const = 0;
    float compute_t(const IterResult& r) const;
};

} // namespace artgen

#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"
#include <cstdint>

namespace artgen {

// Cyclic Cellular Automaton (Griffeath / Fisch-Gravner-Griffeath).
//
// Each cell holds an integer state in [0, states-1].  At every step a cell in
// state c advances to (c+1) % states if any neighbor already holds that value.
// All cells update simultaneously (double-buffered).
//
// Starting from random noise the grid spontaneously organises into spiraling
// waves and geometric patterns.
//
// Neighborhood choices:
//   Moore (8-neighbor)      — square, geometric crystals
//   Von Neumann (4-neighbor) — diamond-shaped spiral waves
//
// Not tileable — global grid state.
class CyclicCAAlgorithm : public IAlgorithm {
public:
    int      states = 12;    // N: number of distinct states [2, 255]
    bool     moore  = true;  // true = 8-neighbor Moore; false = 4-neighbor Von Neumann
    int      steps  = 500;
    uint32_t seed   = 42;
    Palette  palette;

    const char* name()        const override { return "cyclic_ca"; }
    bool        is_tileable() const override { return false; }
    void render(PixelBuffer& buf, const Viewport& vp) const override;
    void set_palette_phase(float p) override { palette.phase = p; }
};

} // namespace artgen

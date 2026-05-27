#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"
#include <cstdint>
#include <string>

namespace artgen {

// Gray-Scott reaction-diffusion simulation.
// Produces organic textures: coral, spots, worms, mazes.
// Named parameter sets selectable via preset string.
class ReactionDiffusion : public IAlgorithm {
public:
    // Diffusion rates
    float Du   = 0.2097f;
    float Dv   = 0.1050f;
    // Feed / kill rates (try presets below)
    float feed = 0.0545f;
    float kill = 0.062f;
    float dt   = 1.0f;
    int   steps       = 8000;
    uint32_t seed     = 42;
    Palette  palette;

    ReactionDiffusion();

    // Named presets: "coral" "mitosis" "worms" "maze" "spots" "fingerprint"
    void set_preset(const std::string& name);

    const char* name()       const override { return "reaction_diffusion"; }
    bool is_tileable()       const override { return false; }
    void render(PixelBuffer& buf, const Viewport& vp) const override;
    void set_palette_phase(float p) override { palette.phase = p; }
};

} // namespace artgen

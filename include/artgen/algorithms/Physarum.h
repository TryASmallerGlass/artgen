#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"
#include <cstdint>

namespace artgen {

// Physarum Polycephalum (slime-mould) simulation.
//
// Based on Jeff Jones (2010).  Agents move across a float trail grid in three
// phases each step:
//
//   1. Sense   — sample trail at three forward-facing sensor positions
//                (straight, left ±sensor_angle, right ±sensor_angle at
//                sensor_dist pixels).  Steer toward strongest signal.
//   2. Move    — advance by step_size; deposit `deposit` into trail grid.
//   3. Diffuse — apply a 3×3 box blur then multiply by decay (evaporation).
//
// The final trail is log-normalised and mapped through the palette.
// Not tileable — global trail grid and agent state.
class PhysarumAlgorithm : public IAlgorithm {
public:
    int      num_agents     = 200'000;
    int      steps          = 400;
    float    sensor_angle   = 45.0f;   // degrees, sensor offset left/right
    float    sensor_dist    = 9.0f;    // pixels, how far ahead sensors probe
    float    rotation_angle = 45.0f;   // degrees, turn step when re-orienting
    float    step_size      = 1.0f;    // pixels per move step
    float    deposit        = 5.0f;    // trail amount deposited per step
    float    decay          = 0.95f;   // trail multiplied by this each step
    uint32_t seed           = 42;
    Palette  palette;

    const char* name()        const override { return "physarum"; }
    bool        is_tileable() const override { return false; }
    void render(PixelBuffer& buf, const Viewport& vp) const override;
    void set_palette_phase(float p) override { palette.phase = p; }
};

} // namespace artgen

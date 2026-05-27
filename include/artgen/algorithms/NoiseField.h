#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"
#include <cstdint>

namespace artgen {

// Fractal Brownian Motion (fBm) using 2D Simplex noise.
// Maps viewport coordinates directly into noise space.
class NoiseFieldAlgorithm : public IAlgorithm {
public:
    int      octaves     = 6;
    float    persistence = 0.5f;  // amplitude falloff per octave
    float    lacunarity  = 2.0f;  // frequency growth per octave
    float    scale       = 1.0f;  // overall frequency multiplier
    uint32_t seed        = 42;
    Palette  palette;

    NoiseFieldAlgorithm();
    const char* name() const override { return "noise"; }
    void render(PixelBuffer& buf, const Viewport& vp) const override;
    void set_palette_phase(float p) override { palette.phase = p; }

private:
    float fbm(float x, float y, const int perm[512]) const;
    static float simplex2(float x, float y, const int perm[512]);
    static void  build_perm(int perm[512], uint32_t seed);
};

} // namespace artgen

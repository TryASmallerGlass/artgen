#include "artgen/PaletteGenerator.h"
#include <cmath>
#include <initializer_list>
#include <vector>

namespace artgen {

static float wrap_hue(float h) {
    h = std::fmod(h, 360.0f);
    return h < 0.0f ? h + 360.0f : h;
}

static RGBA hsl_rgba(float h, float s, float l) {
    RGB rgb = hsl_to_rgb({wrap_hue(h), s, l});
    return {rgb.r, rgb.g, rgb.b, 1.0f};
}

Palette PaletteGenerator::from_hues(std::initializer_list<float> hues, float s, float l) {
    Palette p;
    std::vector<float> hvec(hues);
    int n = static_cast<int>(hvec.size());

    // Black intro stop for visual pop
    p.add_stop(0.00f, {0.0f, 0.0f, 0.0f, 1.0f});

    // Each hue gets an evenly spaced band
    for (int i = 0; i < n; ++i) {
        float pos_lo = 0.05f + static_cast<float>(i)     / n * 0.9f;
        float pos_hi = 0.05f + static_cast<float>(i + 1) / n * 0.9f;
        float mid    = (pos_lo + pos_hi) * 0.5f;
        p.add_stop(mid, hsl_rgba(hvec[i], s, l));
    }

    // Black outro stop for seamless cycling
    p.add_stop(1.00f, {0.0f, 0.0f, 0.0f, 1.0f});
    return p;
}

Palette PaletteGenerator::complementary(HSL base) {
    return from_hues({base.h, base.h + 180.0f}, base.s, base.l);
}

Palette PaletteGenerator::triadic(HSL base) {
    return from_hues({base.h, base.h + 120.0f, base.h + 240.0f}, base.s, base.l);
}

Palette PaletteGenerator::analogous(HSL base, float spread_deg) {
    float h = base.h;
    return from_hues({h - spread_deg, h - spread_deg * 0.5f,
                      h, h + spread_deg * 0.5f, h + spread_deg},
                     base.s, base.l);
}

Palette PaletteGenerator::split_complementary(HSL base) {
    return from_hues({base.h, base.h + 150.0f, base.h + 180.0f, base.h + 210.0f},
                     base.s, base.l);
}

} // namespace artgen

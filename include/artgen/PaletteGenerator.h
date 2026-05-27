#pragma once
#include "artgen/Palette.h"
#include "artgen/Color.h"

namespace artgen {

// Generates Palette objects from color-theory rules applied to a base HSL color.
struct PaletteGenerator {
    // Two-tone gradient between base and its complement (hue + 180°)
    static Palette complementary(HSL base);

    // Three-tone gradient cycling through base, base+120°, base+240°
    static Palette triadic(HSL base);

    // Five-stop gradient spanning base ± spread degrees of hue
    static Palette analogous(HSL base, float spread_deg = 30.0f);

    // Four stops: base, base+150°, base+180°, base+210°
    static Palette split_complementary(HSL base);

private:
    static Palette from_hues(std::initializer_list<float> hues, float s, float l);
};

} // namespace artgen

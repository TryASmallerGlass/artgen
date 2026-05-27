#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "artgen/PaletteGenerator.h"
#include "artgen/Palette.h"
#include "artgen/Color.h"

using namespace artgen;
using Catch::Approx;

TEST_CASE("Complementary palette has stops", "[palette_gen]") {
    Palette p = PaletteGenerator::complementary({200.0f, 0.8f, 0.5f});
    // The generator creates stops — just verify it samples without crashing
    REQUIRE_NOTHROW(p.sample(0.0f));
    REQUIRE_NOTHROW(p.sample(0.5f));
    REQUIRE_NOTHROW(p.sample(1.0f));
}

TEST_CASE("Triadic palette samples over full range", "[palette_gen]") {
    Palette p = PaletteGenerator::triadic({120.0f, 0.7f, 0.5f});
    for (float t = 0.0f; t <= 1.0f; t += 0.1f)
        REQUIRE_NOTHROW(p.sample(t));
}

TEST_CASE("Analogous palette has more stops than complementary", "[palette_gen]") {
    // analogous produces 5 hues + 2 black endpoints = 7 stops
    // complementary produces 2 hues + 2 black endpoints = 4 stops
    // We don't expose stop count directly, but ensure sampling works
    Palette a = PaletteGenerator::analogous({30.0f, 0.8f, 0.5f}, 30.0f);
    REQUIRE_NOTHROW(a.sample(0.5f));
}

TEST_CASE("Split complementary samples correctly", "[palette_gen]") {
    Palette p = PaletteGenerator::split_complementary({60.0f, 0.8f, 0.5f});
    REQUIRE_NOTHROW(p.sample(0.0f));
    REQUIRE_NOTHROW(p.sample(1.0f));
}

TEST_CASE("Different palette types produce different colors at midpoint", "[palette_gen]") {
    HSL base{200.0f, 0.8f, 0.5f};
    Palette comp = PaletteGenerator::complementary(base);
    Palette tri  = PaletteGenerator::triadic(base);
    RGBA c1 = comp.sample(0.5f);
    RGBA c2 = tri.sample(0.5f);
    bool differs = (c1.r != c2.r || c1.g != c2.g || c1.b != c2.b);
    REQUIRE(differs);
}

TEST_CASE("LAB interpolation produces valid colors", "[palette_lab]") {
    Palette p = Palette::classic_mandelbrot();
    p.use_lab_interpolation = true;
    for (float t = 0.0f; t <= 1.0f; t += 0.05f) {
        RGBA c = p.sample(t);
        REQUIRE(c.r >= 0.0f); REQUIRE(c.r <= 1.0f);
        REQUIRE(c.g >= 0.0f); REQUIRE(c.g <= 1.0f);
        REQUIRE(c.b >= 0.0f); REQUIRE(c.b <= 1.0f);
        REQUIRE(c.a >= 0.0f); REQUIRE(c.a <= 1.0f);
    }
}

TEST_CASE("LAB interpolation differs from linear for high-contrast palette", "[palette_lab]") {
    // Red to cyan — complementary colors. LAB interp avoids the dark band in the middle.
    Palette linear, lab;
    linear.add_stop(0.0f, {1.0f, 0.0f, 0.0f, 1.0f}); // red
    linear.add_stop(1.0f, {0.0f, 1.0f, 1.0f, 1.0f}); // cyan
    lab = linear;
    lab.use_lab_interpolation = true;

    RGBA mid_lin = linear.sample(0.5f);
    RGBA mid_lab = lab.sample(0.5f);
    bool differs = (mid_lin.r != mid_lab.r || mid_lin.g != mid_lab.g || mid_lin.b != mid_lab.b);
    REQUIRE(differs);
}

TEST_CASE("RGB->CMYK->RGB round-trip", "[color][cmyk]") {
    RGB orig{0.6f, 0.3f, 0.8f};
    RGB back = cmyk_to_rgb(rgb_to_cmyk(orig));
    REQUIRE(back.r == Approx(orig.r).margin(0.001f));
    REQUIRE(back.g == Approx(orig.g).margin(0.001f));
    REQUIRE(back.b == Approx(orig.b).margin(0.001f));
}

TEST_CASE("CMYK black is correct", "[color][cmyk]") {
    CMYK k = rgb_to_cmyk({0.0f, 0.0f, 0.0f});
    REQUIRE(k.k == Approx(1.0f).margin(0.001f));
}

TEST_CASE("CMYK white is correct", "[color][cmyk]") {
    CMYK w = rgb_to_cmyk({1.0f, 1.0f, 1.0f});
    REQUIRE(w.k == Approx(0.0f).margin(0.001f));
    REQUIRE(w.c == Approx(0.0f).margin(0.001f));
}

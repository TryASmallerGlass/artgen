#include <catch2/catch_test_macros.hpp>
#include "artgen/algorithms/LSystem.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"

using namespace artgen;

TEST_CASE("LSystem plant preset renders without crash", "[lsystem]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    LSystemAlgorithm algo;
    algo.set_preset("plant");
    algo.iterations = 3; // fast for testing
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("LSystem dragon preset renders without crash", "[lsystem]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    LSystemAlgorithm algo;
    algo.set_preset("dragon");
    algo.iterations = 6;
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("LSystem sierpinski preset renders without crash", "[lsystem]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    LSystemAlgorithm algo;
    algo.set_preset("sierpinski");
    algo.iterations = 3;
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("LSystem fills background color", "[lsystem]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    LSystemAlgorithm algo;
    algo.set_preset("plant");
    algo.iterations = 1;
    algo.bg_color = {0.5f, 0.0f, 0.0f, 1.0f};
    algo.fg_color = {0.0f, 1.0f, 0.0f, 1.0f};
    PixelBuffer buf(32, 32);
    algo.render(buf, vp);
    // Background should be the bg_color (check corner pixel far from drawing)
    // At least some pixels should be bg_color
    bool found_bg = false;
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x) {
            RGBA p = buf.get(x, y);
            if (p.r > 0.4f && p.g < 0.1f) { found_bg = true; break; }
        }
    REQUIRE(found_bg);
}

TEST_CASE("LSystem draws foreground lines", "[lsystem]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    LSystemAlgorithm algo;
    algo.axiom      = "F";
    algo.rules      = {};     // no expansion
    algo.iterations = 1;
    algo.angle_deg  = 90.0f;
    algo.fg_color   = {1.0f, 1.0f, 1.0f, 1.0f};
    algo.bg_color   = {0.0f, 0.0f, 0.0f, 1.0f};
    PixelBuffer buf(64, 64);
    algo.render(buf, vp);
    // There should be at least one white pixel
    bool found_fg = false;
    for (int y = 0; y < 64 && !found_fg; ++y)
        for (int x = 0; x < 64 && !found_fg; ++x) {
            RGBA p = buf.get(x, y);
            if (p.r > 0.9f && p.g > 0.9f) found_fg = true;
        }
    REQUIRE(found_fg);
}

TEST_CASE("LSystem is deterministic", "[lsystem]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    LSystemAlgorithm algo;
    algo.set_preset("plant");
    algo.iterations = 3;
    PixelBuffer b1(32, 32), b2(32, 32);
    algo.render(b1, vp);
    algo.render(b2, vp);
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x) {
            REQUIRE(b1.get(x,y).r == b2.get(x,y).r);
            REQUIRE(b1.get(x,y).g == b2.get(x,y).g);
        }
}

TEST_CASE("LSystem different presets produce different output", "[lsystem]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    LSystemAlgorithm a, b;
    a.set_preset("plant");     a.iterations = 3;
    b.set_preset("sierpinski"); b.iterations = 3;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, vp); b.render(bb, vp);
    bool differs = false;
    for (int y = 0; y < 32 && !differs; ++y)
        for (int x = 0; x < 32 && !differs; ++x)
            if (ba.get(x,y).r != bb.get(x,y).r ||
                ba.get(x,y).g != bb.get(x,y).g) differs = true;
    REQUIRE(differs);
}

TEST_CASE("LSystem is_tileable returns false", "[lsystem]") {
    LSystemAlgorithm algo;
    REQUIRE(algo.is_tileable() == false);
}

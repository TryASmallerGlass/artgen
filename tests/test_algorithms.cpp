#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "artgen/algorithms/Mandelbrot.h"
#include "artgen/algorithms/Julia.h"
#include "artgen/algorithms/BurningShip.h"
#include "artgen/algorithms/Newton.h"
#include "artgen/algorithms/NoiseField.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"

using namespace artgen;
using Catch::Approx;

// ── Julia ─────────────────────────────────────────────────────────────────────

TEST_CASE("Julia renders without crash", "[julia]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    JuliaAlgorithm algo;
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("Julia is deterministic", "[julia]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    JuliaAlgorithm algo;
    PixelBuffer buf1(32, 32), buf2(32, 32);
    algo.render(buf1, vp);
    algo.render(buf2, vp);
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x) {
            REQUIRE(buf1.get(x,y).r == buf2.get(x,y).r);
            REQUIRE(buf1.get(x,y).g == buf2.get(x,y).g);
        }
}

TEST_CASE("Julia exterior pixel escapes quickly", "[julia]") {
    // z_0 = (3, 0) is outside the escape radius for any reasonable seed
    Viewport vp;
    vp.pixel_width = 1; vp.pixel_height = 1;
    vp.real_min = vp.real_max = 3.0;
    vp.imag_min = vp.imag_max = 0.0;
    JuliaAlgorithm algo;
    algo.max_iterations = 1000;
    PixelBuffer buf(1, 1);
    algo.render(buf, vp);
    RGBA px = buf.get(0, 0);
    bool non_black = (px.r > 0.01f || px.g > 0.01f || px.b > 0.01f);
    REQUIRE(non_black);
}

TEST_CASE("Julia seed change produces different image", "[julia]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    JuliaAlgorithm a, b;
    a.seed_r = -0.7; a.seed_i = 0.27015;
    b.seed_r =  0.285; b.seed_i = 0.01;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, vp); b.render(bb, vp);
    bool differs = false;
    for (int y = 0; y < 32 && !differs; ++y)
        for (int x = 0; x < 32 && !differs; ++x)
            if (ba.get(x,y).r != bb.get(x,y).r) differs = true;
    REQUIRE(differs);
}

// ── BurningShip ───────────────────────────────────────────────────────────────

TEST_CASE("BurningShip renders without crash", "[burningship]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    BurningShipAlgorithm algo;
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("BurningShip exterior point escapes", "[burningship]") {
    Viewport vp;
    vp.pixel_width = 1; vp.pixel_height = 1;
    vp.real_min = vp.real_max = 3.0;
    vp.imag_min = vp.imag_max = 0.0;
    BurningShipAlgorithm algo;
    PixelBuffer buf(1, 1);
    algo.render(buf, vp);
    RGBA px = buf.get(0, 0);
    REQUIRE((px.r > 0.01f || px.g > 0.01f || px.b > 0.01f));
}

TEST_CASE("BurningShip differs from Mandelbrot", "[burningship]") {
    Viewport vp;
    vp.pixel_width = 32; vp.pixel_height = 32;
    vp.real_min = -2.5; vp.real_max = 1.0;
    vp.imag_min = -1.25; vp.imag_max = 1.25;
    MandelbrotAlgorithm mb;
    BurningShipAlgorithm bs;
    // Give them the same palette
    bs.palette = Palette::classic_mandelbrot();
    PixelBuffer bm(32, 32), bb(32, 32);
    mb.render(bm, vp); bs.render(bb, vp);
    bool differs = false;
    for (int y = 0; y < 32 && !differs; ++y)
        for (int x = 0; x < 32 && !differs; ++x)
            if (bm.get(x,y).r != bb.get(x,y).r) differs = true;
    REQUIRE(differs);
}

// ── Newton ────────────────────────────────────────────────────────────────────

TEST_CASE("Newton renders without crash", "[newton]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    NewtonAlgorithm algo;
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("Newton is deterministic", "[newton]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    NewtonAlgorithm algo;
    PixelBuffer buf1(32, 32), buf2(32, 32);
    algo.render(buf1, vp);
    algo.render(buf2, vp);
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x) {
            REQUIRE(buf1.get(x,y).r == buf2.get(x,y).r);
            REQUIRE(buf1.get(x,y).g == buf2.get(x,y).g);
            REQUIRE(buf1.get(x,y).b == buf2.get(x,y).b);
        }
}

TEST_CASE("Newton power 4 differs from power 3", "[newton]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    NewtonAlgorithm a3, a4;
    a3.power = 3; a4.power = 4;
    PixelBuffer b3(32, 32), b4(32, 32);
    a3.render(b3, vp); a4.render(b4, vp);
    bool differs = false;
    for (int y = 0; y < 32 && !differs; ++y)
        for (int x = 0; x < 32 && !differs; ++x)
            if (b3.get(x,y).r != b4.get(x,y).r) differs = true;
    REQUIRE(differs);
}

// ── NoiseField ────────────────────────────────────────────────────────────────

TEST_CASE("NoiseField renders without crash", "[noise]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    NoiseFieldAlgorithm algo;
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("NoiseField is deterministic", "[noise]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    NoiseFieldAlgorithm algo;
    PixelBuffer buf1(32, 32), buf2(32, 32);
    algo.render(buf1, vp);
    algo.render(buf2, vp);
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x) {
            REQUIRE(buf1.get(x,y).r == buf2.get(x,y).r);
            REQUIRE(buf1.get(x,y).g == buf2.get(x,y).g);
        }
}

TEST_CASE("NoiseField seed change produces different image", "[noise]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    NoiseFieldAlgorithm a, b;
    a.seed = 42; b.seed = 999;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, vp); b.render(bb, vp);
    bool differs = false;
    for (int y = 0; y < 32 && !differs; ++y)
        for (int x = 0; x < 32 && !differs; ++x)
            if (ba.get(x,y).r != bb.get(x,y).r) differs = true;
    REQUIRE(differs);
}

// ── Coloring modes ────────────────────────────────────────────────────────────

TEST_CASE("HistogramEq mode renders without crash", "[coloring]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    MandelbrotAlgorithm algo;
    algo.coloring_mode = ColoringMode::HistogramEq;
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("OrbitTrap mode renders without crash", "[coloring]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    MandelbrotAlgorithm algo;
    algo.coloring_mode = ColoringMode::OrbitTrap;
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("HistogramEq differs from Smooth coloring", "[coloring]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    MandelbrotAlgorithm smooth, hist;
    smooth.coloring_mode = ColoringMode::Smooth;
    hist.coloring_mode   = ColoringMode::HistogramEq;
    PixelBuffer bs(64, 64), bh(64, 64);
    smooth.render(bs, vp); hist.render(bh, vp);
    bool differs = false;
    for (int y = 0; y < 64 && !differs; ++y)
        for (int x = 0; x < 64 && !differs; ++x)
            if (bs.get(x,y).r != bh.get(x,y).r) differs = true;
    REQUIRE(differs);
}

#include <catch2/catch_test_macros.hpp>
#include "artgen/algorithms/Mandelbrot.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"

using namespace artgen;

TEST_CASE("Mandelbrot renders without crash", "[mandelbrot]") {
    Viewport vp;
    vp.pixel_width = 64; vp.pixel_height = 64;
    MandelbrotAlgorithm algo;
    PixelBuffer buf(64, 64);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("Mandelbrot interior (origin) is black", "[mandelbrot]") {
    // (0,0) is inside the set — maps to t=0 → first stop of classic palette = black
    Viewport vp;
    vp.pixel_width = 1; vp.pixel_height = 1;
    vp.real_min = vp.real_max = 0.0;
    vp.imag_min = vp.imag_max = 0.0;
    MandelbrotAlgorithm algo;
    PixelBuffer buf(1, 1);
    algo.render(buf, vp);
    RGBA px = buf.get(0, 0);
    REQUIRE(px.r < 0.01f);
    REQUIRE(px.g < 0.01f);
    REQUIRE(px.b < 0.01f);
}

TEST_CASE("Mandelbrot exterior point is not black", "[mandelbrot]") {
    // (2.5, 0) escapes on the first iteration
    Viewport vp;
    vp.pixel_width = 1; vp.pixel_height = 1;
    vp.real_min = vp.real_max = 2.5;
    vp.imag_min = vp.imag_max = 0.0;
    MandelbrotAlgorithm algo;
    PixelBuffer buf(1, 1);
    algo.render(buf, vp);
    RGBA px = buf.get(0, 0);
    bool non_black = (px.r > 0.01f || px.g > 0.01f || px.b > 0.01f);
    REQUIRE(non_black);
}

TEST_CASE("Mandelbrot output is deterministic", "[mandelbrot]") {
    Viewport vp;
    vp.pixel_width = 32; vp.pixel_height = 32;
    MandelbrotAlgorithm algo;
    PixelBuffer buf1(32, 32), buf2(32, 32);
    algo.render(buf1, vp);
    algo.render(buf2, vp);
    for (int y = 0; y < 32; ++y)
        for (int x = 0; x < 32; ++x) {
            RGBA a = buf1.get(x, y);
            RGBA b = buf2.get(x, y);
            REQUIRE(a.r == b.r);
            REQUIRE(a.g == b.g);
            REQUIRE(a.b == b.b);
            REQUIRE(a.a == b.a);
        }
}

TEST_CASE("Mandelbrot palette swap works", "[mandelbrot]") {
    Viewport vp;
    vp.pixel_width = 32; vp.pixel_height = 32;
    MandelbrotAlgorithm algo_a, algo_b;
    algo_a.palette = Palette::fire();
    algo_b.palette = Palette::ice();
    PixelBuffer buf_a(32, 32), buf_b(32, 32);
    algo_a.render(buf_a, vp);
    algo_b.render(buf_b, vp);
    // At least some exterior pixel should differ between palettes
    bool differs = false;
    for (int y = 0; y < 32 && !differs; ++y)
        for (int x = 0; x < 32 && !differs; ++x) {
            RGBA a = buf_a.get(x, y);
            RGBA b = buf_b.get(x, y);
            if (a.r != b.r || a.g != b.g || a.b != b.b) differs = true;
        }
    REQUIRE(differs);
}

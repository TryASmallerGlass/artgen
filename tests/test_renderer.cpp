#include <catch2/catch_test_macros.hpp>
#include "artgen/Renderer.h"
#include "artgen/algorithms/Mandelbrot.h"
#include "artgen/algorithms/ReactionDiffusion.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"

using namespace artgen;

TEST_CASE("TileRenderer single-thread matches direct render", "[renderer]") {
    Viewport vp; vp.pixel_width = 64; vp.pixel_height = 64;
    MandelbrotAlgorithm algo;

    PixelBuffer direct(64, 64);
    algo.render(direct, vp);

    PixelBuffer tiled(64, 64);
    TileRenderer r;
    r.thread_count  = 1;
    r.tile_size     = 16;
    r.aa_samples    = 1;
    r.show_progress = false;
    r.render(algo, tiled, vp);

    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x) {
            REQUIRE(direct.get(x,y).r == tiled.get(x,y).r);
            REQUIRE(direct.get(x,y).g == tiled.get(x,y).g);
            REQUIRE(direct.get(x,y).b == tiled.get(x,y).b);
        }
}

TEST_CASE("TileRenderer multi-thread matches single-thread", "[renderer]") {
    Viewport vp; vp.pixel_width = 128; vp.pixel_height = 128;
    MandelbrotAlgorithm algo;

    PixelBuffer st(128, 128), mt(128, 128);
    TileRenderer r;
    r.tile_size     = 32;
    r.aa_samples    = 1;
    r.show_progress = false;

    r.thread_count = 1; r.render(algo, st, vp);
    r.thread_count = 4; r.render(algo, mt, vp);

    for (int y = 0; y < 128; ++y)
        for (int x = 0; x < 128; ++x) {
            REQUIRE(st.get(x,y).r == mt.get(x,y).r);
            REQUIRE(st.get(x,y).g == mt.get(x,y).g);
        }
}

TEST_CASE("TileRenderer AA=2 differs from AA=1", "[renderer]") {
    Viewport vp;
    vp.pixel_width = 32; vp.pixel_height = 32;
    vp.real_min = -2.5; vp.real_max = 1.0;
    vp.imag_min = -1.25; vp.imag_max = 1.25;
    MandelbrotAlgorithm algo;

    PixelBuffer noaa(32, 32), aa(32, 32);
    TileRenderer r;
    r.tile_size     = 16;
    r.show_progress = false;
    r.thread_count  = 1;

    r.aa_samples = 1; r.render(algo, noaa, vp);
    r.aa_samples = 2; r.render(algo, aa,   vp);

    bool differs = false;
    for (int y = 0; y < 32 && !differs; ++y)
        for (int x = 0; x < 32 && !differs; ++x)
            if (noaa.get(x,y).r != aa.get(x,y).r) differs = true;
    REQUIRE(differs);
}

TEST_CASE("TileRenderer handles image size not divisible by tile_size", "[renderer]") {
    Viewport vp; vp.pixel_width = 50; vp.pixel_height = 37;
    MandelbrotAlgorithm algo;
    PixelBuffer buf(50, 37);
    TileRenderer r;
    r.tile_size     = 16;
    r.thread_count  = 1;
    r.show_progress = false;
    REQUIRE_NOTHROW(r.render(algo, buf, vp));
}

TEST_CASE("ReactionDiffusion renders without crash (small grid)", "[rd]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    ReactionDiffusion algo;
    algo.steps = 50; // fast for testing
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("ReactionDiffusion is deterministic", "[rd]") {
    Viewport vp; vp.pixel_width = 16; vp.pixel_height = 16;
    ReactionDiffusion algo;
    algo.steps = 20;
    PixelBuffer b1(16, 16), b2(16, 16);
    algo.render(b1, vp);
    algo.render(b2, vp);
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            REQUIRE(b1.get(x,y).r == b2.get(x,y).r);
            REQUIRE(b1.get(x,y).g == b2.get(x,y).g);
        }
}

TEST_CASE("ReactionDiffusion preset modifies feed/kill rates", "[rd]") {
    ReactionDiffusion rd;
    float default_feed = rd.feed;
    float default_kill = rd.kill;
    rd.set_preset("worms");
    // Worms preset uses different rates than default coral
    REQUIRE(rd.feed != default_feed);
    REQUIRE(rd.kill != default_kill);
}

TEST_CASE("ReactionDiffusion simulation produces non-uniform output", "[rd]") {
    Viewport vp; vp.pixel_width = 32; vp.pixel_height = 32;
    ReactionDiffusion algo;
    algo.steps = 200;
    PixelBuffer buf(32, 32);
    algo.render(buf, vp);
    // After sufficient steps with seeds, not all pixels should be identical
    RGBA first = buf.get(0, 0);
    bool non_uniform = false;
    for (int y = 0; y < 32 && !non_uniform; ++y)
        for (int x = 0; x < 32 && !non_uniform; ++x) {
            RGBA p = buf.get(x, y);
            if (p.r != first.r || p.g != first.g || p.b != first.b)
                non_uniform = true;
        }
    REQUIRE(non_uniform);
}

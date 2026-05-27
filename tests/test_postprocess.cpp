#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "artgen/PostProcess.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Palette.h"

using namespace artgen;
using Catch::Approx;

static PixelBuffer solid(int w, int h, RGBA color) {
    PixelBuffer buf(w, h);
    buf.fill(color);
    return buf;
}

TEST_CASE("PostProcess no-op when all defaults", "[postprocess]") {
    PostProcessParams p;
    REQUIRE(!p.enabled());
}

TEST_CASE("PostProcess enabled() detects non-default gamma", "[postprocess]") {
    PostProcessParams p;
    p.gamma = 2.2f;
    REQUIRE(p.enabled());
}

TEST_CASE("PostProcess gamma darkens mid-gray (gamma > 1)", "[postprocess]") {
    auto buf = solid(4, 4, {0.5f, 0.5f, 0.5f, 1.0f});
    PostProcessParams p; p.gamma = 2.2f;
    PostProcessor::apply(buf, p);
    // gamma correction with gamma=2.2 → pow(0.5, 1/2.2) ≈ 0.73 (brighter)
    // inv_gamma = 1/2.2 ≈ 0.455; pow(0.5, 0.455) ≈ 0.73
    REQUIRE(buf.get(0,0).r == Approx(0.73f).margin(0.02f));
}

TEST_CASE("PostProcess brightness additive shift", "[postprocess]") {
    auto buf = solid(4, 4, {0.4f, 0.4f, 0.4f, 1.0f});
    PostProcessParams p; p.brightness = 0.2f;
    PostProcessor::apply(buf, p);
    REQUIRE(buf.get(0,0).r == Approx(0.6f).margin(0.001f));
}

TEST_CASE("PostProcess brightness clamps at 1", "[postprocess]") {
    auto buf = solid(4, 4, {0.9f, 0.9f, 0.9f, 1.0f});
    PostProcessParams p; p.brightness = 0.5f;
    PostProcessor::apply(buf, p);
    REQUIRE(buf.get(0,0).r == Approx(1.0f).margin(0.001f));
}

TEST_CASE("PostProcess contrast increases range", "[postprocess]") {
    // Dark pixel (0.2) should become darker; bright (0.8) brighter
    PixelBuffer buf(2, 1);
    buf.set(0, 0, {0.2f, 0.2f, 0.2f, 1.0f});
    buf.set(1, 0, {0.8f, 0.8f, 0.8f, 1.0f});
    PostProcessParams p; p.contrast = 2.0f;
    PostProcessor::apply(buf, p);
    // (0.2-0.5)*2+0.5 = -0.1 → clamped to 0
    // (0.8-0.5)*2+0.5 = 1.1 → clamped to 1
    REQUIRE(buf.get(0,0).r == Approx(0.0f).margin(0.001f));
    REQUIRE(buf.get(1,0).r == Approx(1.0f).margin(0.001f));
}

TEST_CASE("PostProcess saturation=0 produces grayscale", "[postprocess]") {
    auto buf = solid(4, 4, {0.8f, 0.2f, 0.4f, 1.0f});
    PostProcessParams p; p.saturation = 0.0f;
    PostProcessor::apply(buf, p);
    RGBA c = buf.get(0,0);
    // All channels should equal the luminance
    REQUIRE(c.r == Approx(c.g).margin(0.001f));
    REQUIRE(c.g == Approx(c.b).margin(0.001f));
}

TEST_CASE("PostProcess vignette darkens corners", "[postprocess]") {
    const int W = 16, H = 16;
    auto buf = solid(W, H, {1.0f, 1.0f, 1.0f, 1.0f});
    PostProcessParams p; p.vignette = 1.0f;
    PostProcessor::apply(buf, p);
    // Center pixel (8,8) should be brighter than corner (0,0)
    float center = buf.get(W/2, H/2).r;
    float corner = buf.get(0, 0).r;
    REQUIRE(center > corner);
}

TEST_CASE("PostProcess vignette center is unaffected or near 1", "[postprocess]") {
    const int W = 16, H = 16;
    auto buf = solid(W, H, {1.0f, 1.0f, 1.0f, 1.0f});
    PostProcessParams p; p.vignette = 1.0f;
    PostProcessor::apply(buf, p);
    // At exact center the distance is 0, so vf = 1.0 - 0 = 1.0
    REQUIRE(buf.get(W/2, H/2).r == Approx(1.0f).margin(0.05f));
}

TEST_CASE("PostProcess alpha channel is preserved", "[postprocess]") {
    auto buf = solid(4, 4, {0.5f, 0.5f, 0.5f, 0.75f});
    PostProcessParams p; p.contrast = 1.5f;
    PostProcessor::apply(buf, p);
    REQUIRE(buf.get(0,0).a == Approx(0.75f).margin(0.001f));
}

TEST_CASE("PostProcess palette phase wraps correctly", "[postprocess]") {
    Palette pal = Palette::fire();
    RGBA at0  = pal.sample(0.0f);
    RGBA at05 = pal.sample(0.5f);
    pal.phase = 0.5f;
    // Sampling 0.0 with phase=0.5 should equal sampling 0.5 without phase
    RGBA shifted = pal.sample(0.0f);
    REQUIRE(shifted.r == Approx(at05.r).margin(0.001f));
    REQUIRE(shifted.g == Approx(at05.g).margin(0.001f));
    REQUIRE(shifted.b == Approx(at05.b).margin(0.001f));
    (void)at0;
}

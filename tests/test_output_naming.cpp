#include <catch2/catch_test_macros.hpp>
#include "artgen/OutputNaming.h"
#include "artgen/PixelBuffer.h"
#include <regex>
#include <string>

using namespace artgen;

// ── make_output_stem ─────────────────────────────────────────────────────────

TEST_CASE("make_output_stem returns non-empty string", "[output_naming]") {
    std::string stem = make_output_stem("mandelbrot");
    REQUIRE_FALSE(stem.empty());
}

TEST_CASE("make_output_stem includes algorithm name prefix", "[output_naming]") {
    // make_output_stem uses first 7 chars of the algo name as prefix
    std::string stem = make_output_stem("mandelbrot");
    REQUIRE(stem.find("madelb") == std::string::npos);  // not this
    REQUIRE(stem.substr(0, 7) == "mandelb");
}

TEST_CASE("make_output_stem matches expected format", "[output_naming]") {
    // Expected: <7-char-prefix>_YYYYMMDD_HHMM_NNNNN
    std::string stem = make_output_stem("nova");
    std::regex pattern(R"(nova_\d{8}_\d{4}_\d+)");
    REQUIRE(std::regex_search(stem, pattern));
}

TEST_CASE("make_output_stem is unique across calls", "[output_naming]") {
    // Two consecutive calls should produce different stems (counter increments)
    std::string a = make_output_stem("julia");
    std::string b = make_output_stem("julia");
    REQUIRE(a != b);
}

TEST_CASE("make_output_stem uses provided algorithm name", "[output_naming]") {
    std::string stem = make_output_stem("nova");
    REQUIRE(stem.find("nova") != std::string::npos);
}

// ── check_render_quality ──────────────────────────────────────────────────────

TEST_CASE("check_render_quality does not throw on normal output", "[output_naming]") {
    PixelBuffer buf(64, 64);
    // Fill with varied, non-black pixels
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            buf.set(x, y, {(float)x / 63.f, (float)y / 63.f, 0.5f, 1.f});
    REQUIRE_NOTHROW(check_render_quality(buf, "test"));
}

TEST_CASE("check_render_quality does not throw on near-black buffer", "[output_naming]") {
    // Should warn to stderr but never throw
    PixelBuffer buf(64, 64);
    // Leave buffer all-black (default constructed)
    REQUIRE_NOTHROW(check_render_quality(buf, "test_black"));
}

TEST_CASE("check_render_quality does not throw on flat uniform buffer", "[output_naming]") {
    // Uniform mid-grey — low variance
    PixelBuffer buf(64, 64);
    for (int y = 0; y < 64; ++y)
        for (int x = 0; x < 64; ++x)
            buf.set(x, y, {0.5f, 0.5f, 0.5f, 1.f});
    REQUIRE_NOTHROW(check_render_quality(buf, "test_flat"));
}

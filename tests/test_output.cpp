#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "artgen/output/ExrWriter.h"
#include "artgen/AcoImport.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Palette.h"
#include "artgen/config/SceneConfig.h"
#include <cstdio>
#include <fstream>
#include <string>

using namespace artgen;
using Catch::Approx;

// ── EXR writer ────────────────────────────────────────────────────────────────

TEST_CASE("ExrWriter writes without crash (solid color)", "[exr]") {
    PixelBuffer buf(16, 16);
    buf.fill({0.5f, 0.25f, 0.75f, 1.0f});
    REQUIRE_NOTHROW(ExrWriter::write(buf, "test_output.exr"));
    std::remove("test_output.exr");
}

TEST_CASE("ExrWriter writes fp16 variant without crash", "[exr]") {
    PixelBuffer buf(8, 8);
    buf.fill({1.0f, 0.0f, 0.0f, 1.0f});
    REQUIRE_NOTHROW(ExrWriter::write(buf, "test_output_fp16.exr", true));
    std::remove("test_output_fp16.exr");
}

TEST_CASE("ExrWriter creates file on disk", "[exr]") {
    PixelBuffer buf(4, 4);
    buf.fill({0.0f, 1.0f, 0.0f, 1.0f});
    std::string path = "test_exr_exists.exr";
    ExrWriter::write(buf, path);
    std::ifstream f(path, std::ios::binary);
    REQUIRE(f.is_open());
    f.close();
    std::remove(path.c_str());
}

TEST_CASE("ExrWriter throws on bad path", "[exr]") {
    PixelBuffer buf(4, 4);
    REQUIRE_THROWS(ExrWriter::write(buf, "Z:/no_such_dir/bad.exr"));
}

// ── ACO import ────────────────────────────────────────────────────────────────

// Build a minimal version-1 .aco file in memory
static std::string make_temp_aco(const std::string& base) {
    std::string path = base + ".aco";
    std::ofstream f(path, std::ios::binary);

    auto w16 = [&](uint16_t v) {
        uint8_t b[2] = {static_cast<uint8_t>(v >> 8),
                         static_cast<uint8_t>(v & 0xFF)};
        f.write(reinterpret_cast<char*>(b), 2);
    };

    // Version 1
    w16(1);
    // 3 swatches
    w16(3);
    // Swatch 0: RGB red   (65535, 0, 0, 0)
    w16(0); w16(65535); w16(0); w16(0); w16(0);
    // Swatch 1: RGB green (0, 65535, 0, 0)
    w16(0); w16(0); w16(65535); w16(0); w16(0);
    // Swatch 2: RGB blue  (0, 0, 65535, 0)
    w16(0); w16(0); w16(0); w16(65535); w16(0);

    return path;
}

TEST_CASE("load_aco loads version-1 RGB swatches", "[aco]") {
    std::string path = make_temp_aco("test_v1");
    Palette pal;
    REQUIRE_NOTHROW(pal = load_aco(path));
    std::remove(path.c_str());

    // Stop at t=0 should be red
    RGBA c0 = pal.sample(0.0f);
    REQUIRE(c0.r == Approx(1.0f).margin(0.01f));
    REQUIRE(c0.g == Approx(0.0f).margin(0.01f));
    REQUIRE(c0.b == Approx(0.0f).margin(0.01f));

    // Stop at t=1 should be blue
    RGBA c1 = pal.sample(1.0f);
    REQUIRE(c1.b == Approx(1.0f).margin(0.01f));
}

TEST_CASE("load_aco throws for non-existent file", "[aco]") {
    REQUIRE_THROWS_AS(load_aco("no_such_file_xyz.aco"), std::runtime_error);
}

TEST_CASE("load_aco produces a palette with correct stop count", "[aco]") {
    std::string path = make_temp_aco("test_stops");
    Palette pal = load_aco(path);
    std::remove(path.c_str());

    // 3 swatches → stops at 0, 0.5, 1.0
    // Midpoint sample should be green
    RGBA mid = pal.sample(0.5f);
    REQUIRE(mid.g == Approx(1.0f).margin(0.01f));
}

TEST_CASE("SceneConfig::build_palette returns usable palette", "[aco]") {
    SceneConfig cfg;
    Palette pal;
    REQUIRE_NOTHROW(pal = cfg.build_palette());
    REQUIRE_NOTHROW(pal.sample(0.5f));
}

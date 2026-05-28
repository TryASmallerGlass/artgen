#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "artgen/algorithms/Multibrot.h"
#include "artgen/algorithms/Tricorn.h"
#include "artgen/algorithms/Attractor.h"
#include "artgen/algorithms/Plasma.h"
#include "artgen/algorithms/Voronoi.h"
#include "artgen/AlgorithmRegistry.h"
#include "artgen/config/SceneConfig.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"

using namespace artgen;
using Catch::Approx;

// ── helpers ───────────────────────────────────────────────────────────────────

static Viewport make_vp(int w = 32, int h = 32) {
    Viewport v;
    v.pixel_width = w; v.pixel_height = h;
    return v;
}

static bool any_non_black(const PixelBuffer& buf) {
    for (int y = 0; y < buf.height(); ++y)
        for (int x = 0; x < buf.width(); ++x) {
            RGBA c = buf.get(x, y);
            if (c.r > 0.001f || c.g > 0.001f || c.b > 0.001f) return true;
        }
    return false;
}

static bool buffers_equal(const PixelBuffer& a, const PixelBuffer& b) {
    if (a.width() != b.width() || a.height() != b.height()) return false;
    for (int y = 0; y < a.height(); ++y)
        for (int x = 0; x < a.width(); ++x) {
            RGBA ca = a.get(x, y), cb = b.get(x, y);
            if (std::abs(ca.r - cb.r) > 1e-4f ||
                std::abs(ca.g - cb.g) > 1e-4f ||
                std::abs(ca.b - cb.b) > 1e-4f) return false;
        }
    return true;
}

// ── Multibrot ─────────────────────────────────────────────────────────────────

TEST_CASE("Multibrot renders without crash (power 3)", "[multibrot]") {
    MultibrotAlgorithm algo;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Multibrot name is multibrot", "[multibrot]") {
    MultibrotAlgorithm algo;
    REQUIRE(std::string(algo.name()) == "multibrot");
}

TEST_CASE("Multibrot power 3 differs from Mandelbrot (power 2)", "[multibrot]") {
    MultibrotAlgorithm m3;
    m3.power = 3.0;

    MultibrotAlgorithm m2;
    m2.power = 2.0;

    PixelBuffer buf3(32, 32), buf2(32, 32);
    m3.render(buf3, make_vp());
    m2.render(buf2, make_vp());
    REQUIRE_FALSE(buffers_equal(buf3, buf2));
}

TEST_CASE("Multibrot power 2 produces same image as itself (deterministic)", "[multibrot]") {
    MultibrotAlgorithm a, b;
    a.power = b.power = 2.0;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("Multibrot produces non-black pixels", "[multibrot]") {
    MultibrotAlgorithm algo;
    PixelBuffer buf(32, 32);
    algo.render(buf, make_vp());
    REQUIRE(any_non_black(buf));
}

TEST_CASE("Registry creates multibrot", "[multibrot]") {
    SceneConfig cfg;
    cfg.algorithm_name  = "multibrot";
    cfg.multibrot_power = 4.0;
    auto algo = AlgorithmRegistry::create("multibrot", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "multibrot");
}

// ── Tricorn ───────────────────────────────────────────────────────────────────

TEST_CASE("Tricorn renders without crash", "[tricorn]") {
    TricornAlgorithm algo;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Tricorn name is tricorn", "[tricorn]") {
    TricornAlgorithm algo;
    REQUIRE(std::string(algo.name()) == "tricorn");
}

TEST_CASE("Tricorn differs from Mandelbrot", "[tricorn]") {
    TricornAlgorithm tricorn;
    MultibrotAlgorithm mandel;
    mandel.power = 2.0;
    PixelBuffer bt(32, 32), bm(32, 32);
    tricorn.render(bt, make_vp());
    mandel.render(bm, make_vp());
    REQUIRE_FALSE(buffers_equal(bt, bm));
}

TEST_CASE("Tricorn is deterministic", "[tricorn]") {
    TricornAlgorithm a, b;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("Registry creates tricorn", "[tricorn]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("tricorn", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "tricorn");
}

// ── Strange Attractor ─────────────────────────────────────────────────────────

TEST_CASE("Attractor (Clifford) renders without crash", "[attractor]") {
    AttractorAlgorithm algo;
    algo.iterations = 50000; // fast for tests
    PixelBuffer buf(32, 32);
    Viewport vp = make_vp();
    vp.real_min = -2.5; vp.real_max = 2.5;
    vp.imag_min = -2.5; vp.imag_max = 2.5;
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("Attractor (De Jong) renders without crash", "[attractor]") {
    AttractorAlgorithm algo;
    algo.type       = AttractorType::DeJong;
    algo.a = 1.641; algo.b = 1.902; algo.c = 0.316; algo.d = 1.525;
    algo.iterations = 50000;
    PixelBuffer buf(32, 32);
    Viewport vp = make_vp();
    vp.real_min = -3.0; vp.real_max = 3.0;
    vp.imag_min = -3.0; vp.imag_max = 3.0;
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("Attractor is not tileable", "[attractor]") {
    AttractorAlgorithm algo;
    REQUIRE_FALSE(algo.is_tileable());
}

TEST_CASE("Attractor (Clifford) produces non-black pixels", "[attractor]") {
    AttractorAlgorithm algo;
    algo.iterations = 100000;
    PixelBuffer buf(32, 32);
    Viewport vp = make_vp();
    vp.real_min = -2.5; vp.real_max = 2.5;
    vp.imag_min = -2.5; vp.imag_max = 2.5;
    algo.render(buf, vp);
    REQUIRE(any_non_black(buf));
}

TEST_CASE("Attractor is deterministic", "[attractor]") {
    Viewport vp = make_vp();
    vp.real_min = -2.5; vp.real_max = 2.5;
    vp.imag_min = -2.5; vp.imag_max = 2.5;

    AttractorAlgorithm a, b;
    a.iterations = b.iterations = 50000;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, vp);
    b.render(bb, vp);
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("Registry creates attractor", "[attractor]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("attractor", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "attractor");
}

// ── Plasma ────────────────────────────────────────────────────────────────────

TEST_CASE("Plasma renders without crash", "[plasma]") {
    PlasmaAlgorithm algo;
    algo.octaves = 4; // small grid for speed
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Plasma name is plasma", "[plasma]") {
    PlasmaAlgorithm algo;
    REQUIRE(std::string(algo.name()) == "plasma");
}

TEST_CASE("Plasma is not tileable", "[plasma]") {
    PlasmaAlgorithm algo;
    REQUIRE_FALSE(algo.is_tileable());
}

TEST_CASE("Plasma produces non-uniform output", "[plasma]") {
    PlasmaAlgorithm algo;
    algo.octaves = 4;
    PixelBuffer buf(32, 32);
    algo.render(buf, make_vp());
    // At least two pixels should differ
    bool found_diff = false;
    RGBA first = buf.get(0, 0);
    for (int y = 0; y < 32 && !found_diff; ++y)
        for (int x = 0; x < 32 && !found_diff; ++x) {
            RGBA c = buf.get(x, y);
            if (std::abs(c.r - first.r) > 0.01f) found_diff = true;
        }
    REQUIRE(found_diff);
}

TEST_CASE("Plasma is deterministic", "[plasma]") {
    PlasmaAlgorithm a, b;
    a.octaves = b.octaves = 4;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("Plasma different seeds produce different output", "[plasma]") {
    PlasmaAlgorithm a, b;
    a.octaves = b.octaves = 4;
    a.seed = 1; b.seed = 2;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE_FALSE(buffers_equal(ba, bb));
}

TEST_CASE("Registry creates plasma", "[plasma]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("plasma", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "plasma");
}

// ── Voronoi ───────────────────────────────────────────────────────────────────

TEST_CASE("Voronoi renders without crash (cells)", "[voronoi]") {
    VoronoiAlgorithm algo;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Voronoi name is voronoi", "[voronoi]") {
    VoronoiAlgorithm algo;
    REQUIRE(std::string(algo.name()) == "voronoi");
}

TEST_CASE("Voronoi distance mode renders without crash", "[voronoi]") {
    VoronoiAlgorithm algo;
    algo.mode = VoronoiMode::Distance;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Voronoi edge mode renders without crash", "[voronoi]") {
    VoronoiAlgorithm algo;
    algo.mode = VoronoiMode::Edge;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Voronoi Manhattan metric renders without crash", "[voronoi]") {
    VoronoiAlgorithm algo;
    algo.metric = VoronoiMetric::Manhattan;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Voronoi Chebyshev metric renders without crash", "[voronoi]") {
    VoronoiAlgorithm algo;
    algo.metric = VoronoiMetric::Chebyshev;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Voronoi is deterministic", "[voronoi]") {
    VoronoiAlgorithm a, b;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("Voronoi different seeds produce different output", "[voronoi]") {
    VoronoiAlgorithm a, b;
    a.seed = 1; b.seed = 99;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE_FALSE(buffers_equal(ba, bb));
}

TEST_CASE("Voronoi modes produce different output", "[voronoi]") {
    VoronoiAlgorithm cells, dist, edge;
    cells.mode = VoronoiMode::Cells;
    dist.mode  = VoronoiMode::Distance;
    edge.mode  = VoronoiMode::Edge;
    PixelBuffer bc(32,32), bd(32,32), be(32,32);
    cells.render(bc, make_vp());
    dist.render(bd, make_vp());
    edge.render(be, make_vp());
    REQUIRE_FALSE(buffers_equal(bc, bd));
    REQUIRE_FALSE(buffers_equal(bc, be));
}

TEST_CASE("Registry creates voronoi", "[voronoi]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("voronoi", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "voronoi");
}

// ── Registry completeness ─────────────────────────────────────────────────────

TEST_CASE("Registry lists all new algorithms", "[registry]") {
    auto names = AlgorithmRegistry::names();
    auto has = [&](const std::string& n) {
        for (const auto& name : names)
            if (name == n) return true;
        return false;
    };
    REQUIRE(has("multibrot"));
    REQUIRE(has("tricorn"));
    REQUIRE(has("attractor"));
    REQUIRE(has("plasma"));
    REQUIRE(has("voronoi"));
}

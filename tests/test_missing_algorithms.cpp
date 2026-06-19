#include <catch2/catch_test_macros.hpp>
#include "artgen/algorithms/Nova.h"
#include "artgen/algorithms/Lyapunov.h"
#include "artgen/algorithms/CyclicCA.h"
#include "artgen/algorithms/Physarum.h"
#include "artgen/algorithms/Attractor.h"
#include "artgen/AlgorithmRegistry.h"
#include "artgen/config/SceneConfig.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"
#include <cmath>

using namespace artgen;

// ── helpers ───────────────────────────────────────────────────────────────────

// Default Palette has no stops → returns black. Add a simple white gradient.
static void set_grayscale(Palette& p) {
    p.add_stop(0.0f, {0.0f, 0.0f, 0.0f, 1.0f});
    p.add_stop(1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
}

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

static bool has_non_uniform(const PixelBuffer& buf) {
    RGBA first = buf.get(0, 0);
    for (int y = 0; y < buf.height(); ++y)
        for (int x = 0; x < buf.width(); ++x) {
            RGBA c = buf.get(x, y);
            if (std::abs(c.r - first.r) > 0.01f ||
                std::abs(c.g - first.g) > 0.01f ||
                std::abs(c.b - first.b) > 0.01f) return true;
        }
    return false;
}

// ── Nova ──────────────────────────────────────────────────────────────────────

TEST_CASE("Nova renders without crash (Mandelbrot mode)", "[nova]") {
    NovaAlgorithm algo;
    algo.max_iterations = 32;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Nova renders without crash (Julia mode)", "[nova]") {
    NovaAlgorithm algo;
    algo.julia_mode = true;
    algo.seed_r = 1.0; algo.seed_i = 0.0;
    algo.max_iterations = 32;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Nova name is nova", "[nova]") {
    NovaAlgorithm algo;
    REQUIRE(std::string(algo.name()) == "nova");
}

TEST_CASE("Nova is tileable", "[nova]") {
    NovaAlgorithm algo;
    REQUIRE(algo.is_tileable());
}

TEST_CASE("Nova is deterministic", "[nova]") {
    NovaAlgorithm a, b;
    a.max_iterations = b.max_iterations = 32;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("Nova Mandelbrot and Julia modes differ", "[nova]") {
    NovaAlgorithm mand, julia;
    mand.julia_mode  = false;
    julia.julia_mode = true;
    mand.max_iterations = julia.max_iterations = 32;
    PixelBuffer bm(32, 32), bj(32, 32);
    mand.render(bm, make_vp());
    julia.render(bj, make_vp());
    REQUIRE_FALSE(buffers_equal(bm, bj));
}

TEST_CASE("Nova produces non-black pixels", "[nova]") {
    NovaAlgorithm algo;
    algo.max_iterations = 64;
    PixelBuffer buf(32, 32);
    algo.render(buf, make_vp());
    REQUIRE(any_non_black(buf));
}

TEST_CASE("Registry creates nova", "[nova]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("nova", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "nova");
}

// ── Lyapunov ──────────────────────────────────────────────────────────────────

TEST_CASE("Lyapunov renders without crash", "[lyapunov]") {
    LyapunovAlgorithm algo;
    algo.iterations = 100;
    algo.warmup     = 50;
    Viewport vp = make_vp();
    vp.real_min = 2.5; vp.real_max = 3.8;
    vp.imag_min = 2.5; vp.imag_max = 3.8;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("Lyapunov name is lyapunov", "[lyapunov]") {
    LyapunovAlgorithm algo;
    REQUIRE(std::string(algo.name()) == "lyapunov");
}

TEST_CASE("Lyapunov is tileable", "[lyapunov]") {
    LyapunovAlgorithm algo;
    REQUIRE(algo.is_tileable());
}

TEST_CASE("Lyapunov is deterministic", "[lyapunov]") {
    Viewport vp = make_vp();
    vp.real_min = 2.5; vp.real_max = 3.8;
    vp.imag_min = 2.5; vp.imag_max = 3.8;
    LyapunovAlgorithm a, b;
    a.iterations = b.iterations = 100;
    a.warmup = b.warmup = 50;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, vp);
    b.render(bb, vp);
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("Lyapunov produces non-black pixels", "[lyapunov]") {
    LyapunovAlgorithm algo;
    algo.iterations = 200;
    algo.warmup = 100;
    set_grayscale(algo.palette);
    Viewport vp = make_vp();
    vp.real_min = 2.5; vp.real_max = 3.5;  // partly stable (negative lambda)
    vp.imag_min = 2.5; vp.imag_max = 3.5;
    PixelBuffer buf(32, 32);
    algo.render(buf, vp);
    REQUIRE(any_non_black(buf));  // stable pixels (lambda < 0) map to t > 0
}

TEST_CASE("Lyapunov sequence AAB differs from ABB", "[lyapunov]") {
    // "AAB" uses a twice as often as b; "ABB" uses b twice as often — different lambda field
    Viewport vp = make_vp();
    vp.real_min = 2.5; vp.real_max = 3.8;
    vp.imag_min = 2.5; vp.imag_max = 3.8;
    LyapunovAlgorithm aab, abb;
    aab.sequence = "AAB";
    abb.sequence = "ABB";
    aab.iterations = abb.iterations = 201;  // odd so A:B ratio differs between sequences
    aab.warmup = abb.warmup = 100;
    set_grayscale(aab.palette);
    set_grayscale(abb.palette);
    PixelBuffer baab(32, 32), babb(32, 32);
    aab.render(baab, vp);
    abb.render(babb, vp);
    REQUIRE_FALSE(buffers_equal(baab, babb));
}

TEST_CASE("Registry creates lyapunov", "[lyapunov]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("lyapunov", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "lyapunov");
}

// ── CyclicCA ──────────────────────────────────────────────────────────────────

TEST_CASE("CyclicCA renders without crash (Moore)", "[cyclic_ca]") {
    CyclicCAAlgorithm algo;
    algo.steps = 20;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("CyclicCA renders without crash (Von Neumann)", "[cyclic_ca]") {
    CyclicCAAlgorithm algo;
    algo.moore = false;
    algo.steps = 20;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("CyclicCA name is cyclic_ca", "[cyclic_ca]") {
    CyclicCAAlgorithm algo;
    REQUIRE(std::string(algo.name()) == "cyclic_ca");
}

TEST_CASE("CyclicCA is not tileable", "[cyclic_ca]") {
    CyclicCAAlgorithm algo;
    REQUIRE_FALSE(algo.is_tileable());
}

TEST_CASE("CyclicCA is deterministic", "[cyclic_ca]") {
    CyclicCAAlgorithm a, b;
    a.steps = b.steps = 20;
    a.seed = b.seed = 42;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("CyclicCA different seeds produce different output", "[cyclic_ca]") {
    // steps=0 renders the raw random initial state; different seeds give different patterns
    CyclicCAAlgorithm a, b;
    a.steps = b.steps = 0;
    a.seed = 1; b.seed = 99;
    set_grayscale(a.palette);
    set_grayscale(b.palette);
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE_FALSE(buffers_equal(ba, bb));
}

TEST_CASE("CyclicCA states=4 differs from states=12", "[cyclic_ca]") {
    CyclicCAAlgorithm a, b;
    a.steps = b.steps = 0;
    a.seed = b.seed = 42;
    a.states = 4; b.states = 12;
    set_grayscale(a.palette);
    set_grayscale(b.palette);
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE_FALSE(buffers_equal(ba, bb));
}

TEST_CASE("Registry creates cyclic_ca", "[cyclic_ca]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("cyclic_ca", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "cyclic_ca");
}

// ── Physarum ──────────────────────────────────────────────────────────────────

TEST_CASE("Physarum renders without crash", "[physarum]") {
    PhysarumAlgorithm algo;
    algo.num_agents = 1000;
    algo.steps = 10;
    PixelBuffer buf(32, 32);
    REQUIRE_NOTHROW(algo.render(buf, make_vp()));
}

TEST_CASE("Physarum name is physarum", "[physarum]") {
    PhysarumAlgorithm algo;
    REQUIRE(std::string(algo.name()) == "physarum");
}

TEST_CASE("Physarum is not tileable", "[physarum]") {
    PhysarumAlgorithm algo;
    REQUIRE_FALSE(algo.is_tileable());
}

TEST_CASE("Physarum is deterministic", "[physarum]") {
    PhysarumAlgorithm a, b;
    a.num_agents = b.num_agents = 1000;
    a.steps = b.steps = 10;
    a.seed = b.seed = 42;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("Physarum different seeds produce different output", "[physarum]") {
    // Use steps=1 so only initial agent positions (seed-dependent) are deposited
    PhysarumAlgorithm a, b;
    a.num_agents = b.num_agents = 500;
    a.steps = b.steps = 1;
    a.deposit = b.deposit = 100.0f;
    a.seed = 1; b.seed = 999;
    set_grayscale(a.palette);
    set_grayscale(b.palette);
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, make_vp());
    b.render(bb, make_vp());
    REQUIRE_FALSE(buffers_equal(ba, bb));
}

TEST_CASE("Physarum produces non-black output", "[physarum]") {
    PhysarumAlgorithm algo;
    algo.num_agents = 2000;
    algo.steps = 10;
    algo.deposit = 50.0f;
    set_grayscale(algo.palette);
    PixelBuffer buf(32, 32);
    algo.render(buf, make_vp());
    REQUIRE(any_non_black(buf));
}

TEST_CASE("Registry creates physarum", "[physarum]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("physarum", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "physarum");
}

// ── Ikeda (via AttractorAlgorithm) ───────────────────────────────────────────

TEST_CASE("Attractor (Ikeda) renders without crash", "[attractor][ikeda]") {
    AttractorAlgorithm algo;
    algo.type = AttractorType::Ikeda;
    algo.u = 0.9;
    algo.iterations = 50000;
    PixelBuffer buf(32, 32);
    Viewport vp = make_vp();
    vp.real_min = -3.0; vp.real_max = 3.0;
    vp.imag_min = -3.0; vp.imag_max = 3.0;
    REQUIRE_NOTHROW(algo.render(buf, vp));
}

TEST_CASE("Attractor (Ikeda) produces non-black pixels", "[attractor][ikeda]") {
    AttractorAlgorithm algo;
    algo.type = AttractorType::Ikeda;
    algo.u = 0.9;
    algo.iterations = 200000;
    PixelBuffer buf(32, 32);
    Viewport vp = make_vp();
    vp.real_min = -3.0; vp.real_max = 3.0;
    vp.imag_min = -3.0; vp.imag_max = 3.0;
    algo.render(buf, vp);
    REQUIRE(any_non_black(buf));
}

TEST_CASE("Attractor (Ikeda) is deterministic", "[attractor][ikeda]") {
    Viewport vp = make_vp();
    vp.real_min = -3.0; vp.real_max = 3.0;
    vp.imag_min = -3.0; vp.imag_max = 3.0;
    AttractorAlgorithm a, b;
    a.type = b.type = AttractorType::Ikeda;
    a.u = b.u = 0.9;
    a.iterations = b.iterations = 50000;
    PixelBuffer ba(32, 32), bb(32, 32);
    a.render(ba, vp);
    b.render(bb, vp);
    REQUIRE(buffers_equal(ba, bb));
}

TEST_CASE("Attractor Ikeda differs from Clifford", "[attractor][ikeda]") {
    Viewport vp = make_vp();
    vp.real_min = -3.0; vp.real_max = 3.0;
    vp.imag_min = -3.0; vp.imag_max = 3.0;
    AttractorAlgorithm ikeda, clifford;
    ikeda.type = AttractorType::Ikeda;
    ikeda.u = 0.9;
    clifford.type = AttractorType::Clifford;
    ikeda.iterations = clifford.iterations = 50000;
    PixelBuffer bi(32, 32), bc(32, 32);
    ikeda.render(bi, vp);
    clifford.render(bc, vp);
    REQUIRE_FALSE(buffers_equal(bi, bc));
}

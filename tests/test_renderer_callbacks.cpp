#include <catch2/catch_test_macros.hpp>
#include "artgen/Renderer.h"
#include "artgen/algorithms/Mandelbrot.h"
#include "artgen/algorithms/Plasma.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"
#include <atomic>

using namespace artgen;

static Viewport make_vp(int w = 32, int h = 32) {
    Viewport v;
    v.pixel_width = w; v.pixel_height = h;
    return v;
}

// ── progress_cb ───────────────────────────────────────────────────────────────

TEST_CASE("TileRenderer progress_cb is called during tileable render", "[renderer][callbacks]") {
    MandelbrotAlgorithm algo;
    PixelBuffer buf(32, 32);
    TileRenderer ren;
    ren.show_progress = false;
    ren.thread_count  = 1;

    int call_count = 0;
    int last_done = 0;
    int last_total = 0;
    ren.progress_cb = [&](int done, int total) {
        ++call_count;
        last_done  = done;
        last_total = total;
    };

    ren.render(algo, buf, make_vp());

    REQUIRE(call_count > 0);
    REQUIRE(last_done == last_total);
    REQUIRE(last_total > 0);
}

TEST_CASE("TileRenderer progress_cb is called during non-tileable render", "[renderer][callbacks]") {
    PlasmaAlgorithm algo;
    PixelBuffer buf(32, 32);
    TileRenderer ren;
    ren.show_progress = false;

    int call_count = 0;
    ren.progress_cb = [&](int done, int total) {
        ++call_count;
        REQUIRE(done == total);  // non-tileable fires once at completion
        REQUIRE(total == 1);
    };

    ren.render(algo, buf, make_vp());

    REQUIRE(call_count == 1);
}

TEST_CASE("TileRenderer with no progress_cb renders normally", "[renderer][callbacks]") {
    MandelbrotAlgorithm algo;
    PixelBuffer buf(32, 32);
    TileRenderer ren;
    ren.show_progress = false;
    // progress_cb is null by default — must not crash
    REQUIRE_NOTHROW(ren.render(algo, buf, make_vp()));
}

// ── cancel_flag ───────────────────────────────────────────────────────────────

TEST_CASE("TileRenderer cancel_flag stops render early", "[renderer][callbacks]") {
    MandelbrotAlgorithm algo;
    // Use a larger buffer so tiles outnumber the early cancel
    PixelBuffer buf(256, 256);
    Viewport vp = make_vp(256, 256);
    TileRenderer ren;
    ren.show_progress = false;
    ren.thread_count  = 1;
    ren.tile_size     = 32;  // 8×8 = 64 tiles

    std::atomic<bool> cancel{false};
    ren.cancel_flag = &cancel;

    int progress_calls = 0;
    ren.progress_cb = [&](int done, int /*total*/) {
        ++progress_calls;
        if (done >= 4) cancel.store(true, std::memory_order_relaxed);
    };

    ren.render(algo, buf, vp);

    // Render stopped early — not all 64 tiles completed
    REQUIRE(progress_calls < 64);
}

TEST_CASE("TileRenderer pre-set cancel_flag skips all tiles", "[renderer][callbacks]") {
    MandelbrotAlgorithm algo;
    PixelBuffer buf(64, 64);
    Viewport vp = make_vp(64, 64);
    TileRenderer ren;
    ren.show_progress = false;

    std::atomic<bool> cancel{true};  // already cancelled
    ren.cancel_flag = &cancel;

    int progress_calls = 0;
    ren.progress_cb = [&](int, int) { ++progress_calls; };

    ren.render(algo, buf, vp);

    REQUIRE(progress_calls == 0);
}

TEST_CASE("TileRenderer with null cancel_flag renders normally", "[renderer][callbacks]") {
    MandelbrotAlgorithm algo;
    PixelBuffer buf(32, 32);
    TileRenderer ren;
    ren.show_progress = false;
    ren.cancel_flag = nullptr;
    REQUIRE_NOTHROW(ren.render(algo, buf, make_vp()));
}

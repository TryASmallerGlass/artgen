#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "artgen/Color.h"

using namespace artgen;
using Catch::Approx;

TEST_CASE("RGB->HSL->RGB round-trip", "[color]") {
    RGB orig{0.5f, 0.25f, 0.75f};
    RGB back = hsl_to_rgb(rgb_to_hsl(orig));
    REQUIRE(back.r == Approx(orig.r).margin(0.001f));
    REQUIRE(back.g == Approx(orig.g).margin(0.001f));
    REQUIRE(back.b == Approx(orig.b).margin(0.001f));
}

TEST_CASE("HSL->RGB known values", "[color]") {
    RGB red = hsl_to_rgb({0.0f, 1.0f, 0.5f});
    REQUIRE(red.r == Approx(1.0f).margin(0.001f));
    REQUIRE(red.g == Approx(0.0f).margin(0.001f));
    REQUIRE(red.b == Approx(0.0f).margin(0.001f));

    RGB white = hsl_to_rgb({0.0f, 0.0f, 1.0f});
    REQUIRE(white.r == Approx(1.0f).margin(0.001f));
    REQUIRE(white.g == Approx(1.0f).margin(0.001f));
    REQUIRE(white.b == Approx(1.0f).margin(0.001f));
}

TEST_CASE("RGB->LAB->RGB round-trip", "[color]") {
    RGB orig{0.8f, 0.4f, 0.2f};
    RGB back = lab_to_rgb(rgb_to_lab(orig));
    REQUIRE(back.r == Approx(orig.r).margin(0.001f));
    REQUIRE(back.g == Approx(orig.g).margin(0.001f));
    REQUIRE(back.b == Approx(orig.b).margin(0.001f));
}

TEST_CASE("RGB->LAB->RGB round-trip (white) satisfies delta_e < 0.01", "[color]") {
    RGB white{1.0f, 1.0f, 1.0f};
    LAB lab  = rgb_to_lab(white);
    LAB lab2 = rgb_to_lab(lab_to_rgb(lab));
    REQUIRE(delta_e(lab, lab2) < 0.01f);
}

TEST_CASE("delta_e is zero for identical colors", "[color]") {
    LAB c{50.0f, 20.0f, -10.0f};
    REQUIRE(delta_e(c, c) == Approx(0.0f));
}

TEST_CASE("RGB->HSV->RGB round-trip", "[color]") {
    RGB orig{0.3f, 0.6f, 0.9f};
    RGB back = hsv_to_rgb(rgb_to_hsv(orig));
    REQUIRE(back.r == Approx(orig.r).margin(0.001f));
    REQUIRE(back.g == Approx(orig.g).margin(0.001f));
    REQUIRE(back.b == Approx(orig.b).margin(0.001f));
}

TEST_CASE("to_u8 clamps and rounds", "[color]") {
    REQUIRE(to_u8(0.0f)  == 0);
    REQUIRE(to_u8(1.0f)  == 255);
    REQUIRE(to_u8(-1.0f) == 0);
    REQUIRE(to_u8(2.0f)  == 255);
}

TEST_CASE("to_u16 clamps and rounds", "[color]") {
    REQUIRE(to_u16(0.0f) == 0);
    REQUIRE(to_u16(1.0f) == 65535);
}

TEST_CASE("lerp midpoint", "[color]") {
    RGBA a{0.0f, 0.0f, 0.0f, 1.0f};
    RGBA b{1.0f, 1.0f, 1.0f, 1.0f};
    RGBA m = lerp(a, b, 0.5f);
    REQUIRE(m.r == Approx(0.5f));
    REQUIRE(m.g == Approx(0.5f));
    REQUIRE(m.b == Approx(0.5f));
    REQUIRE(m.a == Approx(1.0f));
}

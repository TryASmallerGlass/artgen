#include <catch2/catch_test_macros.hpp>
#include "artgen/AlgorithmRegistry.h"
#include "artgen/config/SceneConfig.h"
#include "artgen/IAlgorithm.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"
#include <algorithm>

using namespace artgen;

TEST_CASE("Registry lists built-in algorithms", "[registry]") {
    auto names = AlgorithmRegistry::names();
    REQUIRE(!names.empty());
    auto has = [&](const std::string& n) {
        return std::find(names.begin(), names.end(), n) != names.end();
    };
    REQUIRE(has("mandelbrot"));
    REQUIRE(has("julia"));
    REQUIRE(has("burning_ship"));
    REQUIRE(has("newton"));
    REQUIRE(has("noise"));
    REQUIRE(has("reaction_diffusion"));
    REQUIRE(has("lsystem"));
}

TEST_CASE("Registry has() returns true for known algorithms", "[registry]") {
    REQUIRE(AlgorithmRegistry::has("mandelbrot"));
    REQUIRE(AlgorithmRegistry::has("julia"));
    REQUIRE_FALSE(AlgorithmRegistry::has("nonexistent_xyz"));
}

TEST_CASE("Registry create() returns non-null for mandelbrot", "[registry]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("mandelbrot", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "mandelbrot");
}

TEST_CASE("Registry create() returns nullptr for unknown name", "[registry]") {
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("no_such_algorithm_xyz", cfg);
    REQUIRE(algo == nullptr);
}

TEST_CASE("Registry custom algorithm registers and creates", "[registry]") {
    // Register a trivial custom algorithm
    struct BlackAlgo : IAlgorithm {
        const char* name() const override { return "test_black"; }
        void render(PixelBuffer& buf, const Viewport&) const override {
            buf.fill({0.f, 0.f, 0.f, 1.f});
        }
    };
    AlgorithmRegistry::register_algo("test_black",
        [](const SceneConfig&) -> std::unique_ptr<IAlgorithm> {
            return std::make_unique<BlackAlgo>();
        });
    REQUIRE(AlgorithmRegistry::has("test_black"));
    SceneConfig cfg;
    auto algo = AlgorithmRegistry::create("test_black", cfg);
    REQUIRE(algo != nullptr);

    Viewport vp; vp.pixel_width = 4; vp.pixel_height = 4;
    PixelBuffer buf(4, 4);
    REQUIRE_NOTHROW(algo->render(buf, vp));
    REQUIRE(buf.get(0, 0).r == 0.0f);
}

TEST_CASE("SceneConfig::create_algorithm uses registry for mandelbrot", "[registry]") {
    SceneConfig cfg;
    cfg.algorithm_name = "mandelbrot";
    REQUIRE_NOTHROW(cfg.create_algorithm());
}

TEST_CASE("SceneConfig::create_algorithm throws for unknown algorithm", "[registry]") {
    SceneConfig cfg;
    cfg.algorithm_name = "no_such_algorithm_abc";
    REQUIRE_THROWS_AS(cfg.create_algorithm(), std::runtime_error);
}

TEST_CASE("Registry creates configured julia with seed", "[registry]") {
    SceneConfig cfg;
    cfg.algorithm_name = "julia";
    cfg.julia_cr = -0.7;
    cfg.julia_ci =  0.27015;
    auto algo = AlgorithmRegistry::create("julia", cfg);
    REQUIRE(algo != nullptr);
    REQUIRE(std::string(algo->name()) == "julia");

    Viewport vp; vp.pixel_width = 16; vp.pixel_height = 16;
    PixelBuffer buf(16, 16);
    REQUIRE_NOTHROW(algo->render(buf, vp));
}

TEST_CASE("Registry names() returns sorted list without empty alias", "[registry]") {
    auto names = AlgorithmRegistry::names();
    // Should be sorted
    REQUIRE(std::is_sorted(names.begin(), names.end()));
    // Should not contain the empty-string alias
    for (const auto& n : names)
        REQUIRE(!n.empty());
}

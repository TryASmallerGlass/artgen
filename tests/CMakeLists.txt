add_executable(artgen_tests
    test_color.cpp
    test_mandelbrot.cpp
    test_algorithms.cpp
    test_palette_gen.cpp
    test_renderer.cpp
    test_lsystem.cpp
    test_postprocess.cpp
    test_registry.cpp
    test_output.cpp
    test_new_algorithms.cpp
    test_missing_algorithms.cpp
    test_renderer_callbacks.cpp
    test_output_naming.cpp
)

target_link_libraries(artgen_tests PRIVATE
    artgen_core
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(artgen_tests)

cmake_minimum_required(VERSION 3.24)
project(artgen VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG        v3.11.3
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.6.0
    GIT_SHALLOW    TRUE
)

FetchContent_Declare(
    stb
    GIT_REPOSITORY https://github.com/nothings/stb.git
    GIT_TAG        master
    GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(nlohmann_json glm Catch2 stb)

# ── Optional SDL2 preview window ──────────────────────────────────────────────
option(ARTGEN_PREVIEW "Build interactive SDL2 preview window" ON)

if(ARTGEN_PREVIEW)
    FetchContent_Declare(
        SDL2
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG        release-2.30.9
        GIT_SHALLOW    TRUE
    )
    set(SDL_SHARED   OFF CACHE BOOL "" FORCE)
    set(SDL_STATIC   ON  CACHE BOOL "" FORCE)
    set(SDL_TEST     OFF CACHE BOOL "" FORCE)
    set(SDL2_DISABLE_UNINSTALL ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(SDL2)
endif()

# ── Core library ──────────────────────────────────────────────────────────────
add_library(artgen_core STATIC
    src/core/Color.cpp
    src/core/PixelBuffer.cpp
    src/core/Palette.cpp
    src/core/PaletteGenerator.cpp
    src/core/PostProcess.cpp
    src/core/Renderer.cpp
    src/algorithms/EscapeTime.cpp
    src/algorithms/Mandelbrot.cpp
    src/algorithms/Julia.cpp
    src/algorithms/BurningShip.cpp
    src/algorithms/Newton.cpp
    src/algorithms/NoiseField.cpp
    src/algorithms/ReactionDiffusion.cpp
    src/algorithms/LSystem.cpp
    src/output/PngWriter.cpp
    src/output/TiffWriter.cpp
    src/output/stb_impl.cpp
    src/config/SceneConfig.cpp
)

target_include_directories(artgen_core PUBLIC
    include
    ${stb_SOURCE_DIR}
)

target_link_libraries(artgen_core PUBLIC
    nlohmann_json::nlohmann_json
    glm::glm
)

if(ARTGEN_PREVIEW)
    target_compile_definitions(artgen_core PUBLIC ARTGEN_WITH_SDL2)
    target_sources(artgen_core PRIVATE src/core/Preview.cpp)
    target_link_libraries(artgen_core PUBLIC SDL2::SDL2-static)
    if(WIN32)
        target_link_libraries(artgen_core PUBLIC
            winmm imm32 version setupapi ole32 oleaut32)
    endif()
endif()

if(MSVC)
    target_compile_options(artgen_core PRIVATE /W4)
else()
    target_compile_options(artgen_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(artgen src/main.cpp)
target_link_libraries(artgen PRIVATE artgen_core)

enable_testing()
add_subdirectory(tests)

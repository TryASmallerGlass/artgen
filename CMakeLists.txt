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

FetchContent_Declare(
    tinyexr
    GIT_REPOSITORY https://github.com/syoyo/tinyexr.git
    GIT_TAG        v1.0.9
    GIT_SHALLOW    TRUE
)
set(TINYEXR_BUILD_SAMPLE OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(nlohmann_json glm Catch2 stb tinyexr)

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

# ── Optional Dear ImGui GUI ──────────────────────────────────────────────────
option(ARTGEN_GUI "Build with Dear ImGui GUI" OFF)

if(ARTGEN_GUI)
    if(NOT ARTGEN_PREVIEW)
        message(FATAL_ERROR "ARTGEN_GUI requires ARTGEN_PREVIEW=ON (SDL2 backend)")
    endif()

    FetchContent_Declare(
        imgui
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG        v1.91.5
        GIT_SHALLOW    TRUE
    )
    FetchContent_MakeAvailable(imgui)

    add_library(imgui_lib STATIC
        ${imgui_SOURCE_DIR}/imgui.cpp
        ${imgui_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdl2.cpp
        ${imgui_SOURCE_DIR}/backends/imgui_impl_sdlrenderer2.cpp
    )
    target_include_directories(imgui_lib PUBLIC
        ${imgui_SOURCE_DIR}
        ${imgui_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui_lib PUBLIC SDL2::SDL2-static)
    if(WIN32)
        target_link_libraries(imgui_lib PUBLIC
            winmm imm32 version setupapi ole32 oleaut32)
    endif()
endif()

# ── Core library ──────────────────────────────────────────────────────────────
add_library(artgen_core STATIC
    src/core/Color.cpp
    src/core/PixelBuffer.cpp
    src/core/Palette.cpp
    src/core/PaletteGenerator.cpp
    src/core/PostProcess.cpp
    src/core/Renderer.cpp
    src/core/AlgorithmRegistry.cpp
    src/core/AcoImport.cpp
    src/algorithms/EscapeTime.cpp
    src/algorithms/Mandelbrot.cpp
    src/algorithms/Julia.cpp
    src/algorithms/BurningShip.cpp
    src/algorithms/Newton.cpp
    src/algorithms/NoiseField.cpp
    src/algorithms/ReactionDiffusion.cpp
    src/algorithms/LSystem.cpp
    src/algorithms/Multibrot.cpp
    src/algorithms/Tricorn.cpp
    src/algorithms/Attractor.cpp
    src/algorithms/Plasma.cpp
    src/algorithms/Voronoi.cpp
    src/algorithms/Lyapunov.cpp
    src/algorithms/Nova.cpp
    src/algorithms/CyclicCA.cpp
    src/algorithms/Physarum.cpp
    src/output/PngWriter.cpp
    src/output/TiffWriter.cpp
    src/output/ExrWriter.cpp
    src/output/stb_impl.cpp
    src/config/SceneConfig.cpp
    src/core/OutputNaming.cpp
)

target_include_directories(artgen_core PUBLIC
    include
    ${stb_SOURCE_DIR}
    ${tinyexr_SOURCE_DIR}
)

target_link_libraries(artgen_core PUBLIC
    nlohmann_json::nlohmann_json
    glm::glm
    tinyexr
)

if(MSVC)
    target_compile_options(artgen_core PRIVATE /W4)
else()
    target_compile_options(artgen_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(artgen src/main.cpp)
target_link_libraries(artgen PRIVATE artgen_core)

if(ARTGEN_PREVIEW)
    target_sources(artgen PRIVATE src/preview/Preview.cpp)
    target_compile_definitions(artgen PRIVATE ARTGEN_WITH_SDL2)
    target_link_libraries(artgen PRIVATE SDL2::SDL2-static)
    if(WIN32)
        target_link_libraries(artgen PRIVATE
            winmm imm32 version setupapi ole32 oleaut32)
    endif()
endif()

if(ARTGEN_GUI)
    target_sources(artgen PRIVATE src/gui/Gui.cpp)
    target_compile_definitions(artgen PRIVATE ARTGEN_GUI)
    target_include_directories(artgen PRIVATE ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
    target_link_libraries(artgen PRIVATE imgui_lib)

    # Copy ui/ strings folder next to the executable after every build
    add_custom_command(TARGET artgen POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_SOURCE_DIR}/ui"
            "$<TARGET_FILE_DIR:artgen>/ui"
        COMMENT "Copying ui/ strings to output directory"
    )
endif()

enable_testing()
add_subdirectory(tests)

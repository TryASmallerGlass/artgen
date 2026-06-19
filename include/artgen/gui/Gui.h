#pragma once

namespace artgen::gui {

// Launches the Dear ImGui / SDL2 GUI window.
// Blocks until the user closes the window.
// Returns 0 on clean exit, non-zero on error.
int run();

} // namespace artgen::gui

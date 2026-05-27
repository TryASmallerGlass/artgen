#pragma once
#ifdef ARTGEN_WITH_SDL2

#include "artgen/IAlgorithm.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"

namespace artgen {

struct PreviewOptions {
    int  max_window_w  = 1200;  // cap window size; 0 = match image
    int  max_window_h  = 800;
    int  fast_threads  = 0;     // threads for interactive re-renders (0=hardware)
    bool show_hud      = true;  // key-binding overlay on first open
};

// Open an interactive SDL2 preview window showing the rendered image.
// Controls:
//   Left-drag           pan (shift complex-plane viewport)
//   Scroll-wheel        zoom (toward cursor)
//   ← / →              shift palette phase ±0.05, re-render
//   R                   re-render at current viewport (full quality)
//   S                   save current frame as "preview_save.png"
//   Q / Escape          close window
// Returns when the window is closed.
void run_preview(IAlgorithm& algo, Viewport vp,
                 const PixelBuffer& initial_frame,
                 const PreviewOptions& opts = {});

} // namespace artgen

#endif // ARTGEN_WITH_SDL2

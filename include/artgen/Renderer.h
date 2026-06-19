#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/PixelBuffer.h"
#include "artgen/Viewport.h"
#include <atomic>
#include <functional>

namespace artgen {

class TileRenderer {
public:
    int  thread_count  = 0;    // 0 = hardware_concurrency
    int  tile_size     = 64;
    int  aa_samples    = 1;    // 1=off, 2=2×2 (4 spp), 4=4×4 (16 spp)
    bool show_progress = true;

    // Optional GUI callbacks — nullptr = disabled
    std::function<void(int done, int total)> progress_cb;
    std::atomic<bool>* cancel_flag = nullptr;

    void render(IAlgorithm& algo, PixelBuffer& buf, const Viewport& vp) const;

private:
    struct Tile { int x, y, w, h; };

    static Viewport make_tile_vp(const Viewport& parent,
                                  int tx, int ty, int tw, int th);
    static void copy_tile(PixelBuffer& dst, const PixelBuffer& src, int ox, int oy);
    static void downsample(const PixelBuffer& src, PixelBuffer& dst, int factor);
    void render_tile(IAlgorithm& algo, PixelBuffer& dst,
                     const Viewport& vp, const Tile& t) const;
};

} // namespace artgen

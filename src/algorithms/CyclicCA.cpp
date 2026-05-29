#include "artgen/algorithms/CyclicCA.h"
#include <vector>
#include <cstdint>

namespace artgen {

void CyclicCAAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    const int W = vp.pixel_width;
    const int H = vp.pixel_height;
    const int N = states < 2 ? 2 : (states > 255 ? 255 : states);

    // Simple LCG for reproducible random initialization
    auto lcg = [](uint32_t& s) -> uint32_t {
        s = s * 1664525u + 1013904223u;
        return s;
    };

    // Initialize grid with uniform random states
    std::vector<uint8_t> grid(static_cast<size_t>(W) * H);
    std::vector<uint8_t> next(static_cast<size_t>(W) * H);
    uint32_t rng = seed;
    for (auto& cell : grid)
        cell = static_cast<uint8_t>(lcg(rng) % static_cast<uint32_t>(N));

    // Neighbour offsets
    // Von Neumann (4): N S E W
    // Moore      (8): all 8 surrounding cells
    const int vn_dx[] = { 0,  0, 1, -1 };
    const int vn_dy[] = {-1,  1, 0,  0 };
    const int mo_dx[] = {-1,  0,  1, -1, 1, -1, 0, 1 };
    const int mo_dy[] = {-1, -1, -1,  0, 0,  1, 1, 1 };
    const int n_nb    = moore ? 8 : 4;
    const int* dx     = moore ? mo_dx : vn_dx;
    const int* dy     = moore ? mo_dy : vn_dy;

    // Simulate
    for (int step = 0; step < steps; ++step) {
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                const uint8_t cur  = grid[static_cast<size_t>(y) * W + x];
                const uint8_t want = static_cast<uint8_t>((cur + 1) % N);
                bool advance = false;
                for (int k = 0; k < n_nb && !advance; ++k) {
                    // Toroidal wrap
                    int nx = (x + dx[k] + W) % W;
                    int ny = (y + dy[k] + H) % H;
                    if (grid[static_cast<size_t>(ny) * W + nx] == want)
                        advance = true;
                }
                next[static_cast<size_t>(y) * W + x] = advance ? want : cur;
            }
        }
        grid.swap(next);
    }

    // Visualise: map state → [0,1] → palette
    const float inv_N = 1.0f / static_cast<float>(N - 1);
    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            float t = static_cast<float>(grid[static_cast<size_t>(y) * W + x]) * inv_N;
            buf.set(x, y, palette.sample(t));
        }
    }
}

} // namespace artgen

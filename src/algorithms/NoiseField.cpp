#include "artgen/algorithms/NoiseField.h"
#include <cmath>
#include <algorithm>

namespace artgen {

// 2D gradient table
static const int s_grad[8][2] = {
    {1,1},{-1,1},{1,-1},{-1,-1},{1,0},{-1,0},{0,1},{0,-1}
};

static int fast_floor(float x) {
    return x > 0 ? static_cast<int>(x) : static_cast<int>(x) - 1;
}

float NoiseFieldAlgorithm::simplex2(float x, float y, const int perm[512]) {
    constexpr float F2 = 0.366025403784f; // (sqrt(3)-1)/2
    constexpr float G2 = 0.211324865405f; // (3-sqrt(3))/6

    float s  = (x + y) * F2;
    int   i  = fast_floor(x + s);
    int   j  = fast_floor(y + s);
    float t  = static_cast<float>(i + j) * G2;
    float x0 = x - (i - t);
    float y0 = y - (j - t);

    int i1 = (x0 > y0) ? 1 : 0;
    int j1 = (x0 > y0) ? 0 : 1;

    float x1 = x0 - i1 + G2;
    float y1 = y0 - j1 + G2;
    float x2 = x0 - 1.0f + 2.0f * G2;
    float y2 = y0 - 1.0f + 2.0f * G2;

    int ii  = i & 255;
    int jj  = j & 255;
    int gi0 = perm[ii      + perm[jj     ]] % 8;
    int gi1 = perm[ii + i1 + perm[jj + j1]] % 8;
    int gi2 = perm[ii + 1  + perm[jj + 1 ]] % 8;

    auto contrib = [](float gx, float gy, float px, float py) -> float {
        float t2 = 0.5f - px*px - py*py;
        if (t2 < 0.0f) return 0.0f;
        t2 *= t2;
        return t2 * t2 * (gx * px + gy * py);
    };

    float n = contrib(static_cast<float>(s_grad[gi0][0]), static_cast<float>(s_grad[gi0][1]), x0, y0)
            + contrib(static_cast<float>(s_grad[gi1][0]), static_cast<float>(s_grad[gi1][1]), x1, y1)
            + contrib(static_cast<float>(s_grad[gi2][0]), static_cast<float>(s_grad[gi2][1]), x2, y2);

    return 70.0f * n; // scale to roughly [-1, 1]
}

float NoiseFieldAlgorithm::fbm(float x, float y, const int perm[512]) const {
    float value   = 0.0f;
    float amp     = 1.0f;
    float freq    = scale;
    float max_val = 0.0f;
    for (int i = 0; i < octaves; ++i) {
        value   += simplex2(x * freq, y * freq, perm) * amp;
        max_val += amp;
        amp     *= persistence;
        freq    *= lacunarity;
    }
    return (max_val > 0.0f) ? value / max_val : 0.0f;
}

void NoiseFieldAlgorithm::build_perm(int perm[512], uint32_t seed) {
    int p[256];
    for (int i = 0; i < 256; ++i) p[i] = i;
    uint32_t s = seed;
    for (int i = 255; i > 0; --i) {
        s = s * 1664525u + 1013904223u; // LCG shuffle
        int j = static_cast<int>(s % static_cast<uint32_t>(i + 1));
        std::swap(p[i], p[j]);
    }
    for (int i = 0; i < 512; ++i) perm[i] = p[i & 255];
}

NoiseFieldAlgorithm::NoiseFieldAlgorithm() {
    palette = Palette::classic_mandelbrot();
}

void NoiseFieldAlgorithm::render(PixelBuffer& buf, const Viewport& vp) const {
    int perm[512];
    build_perm(perm, seed);

    for (int py = 0; py < vp.pixel_height; ++py) {
        float wy = static_cast<float>(vp.pixel_to_imag(py));
        for (int px = 0; px < vp.pixel_width; ++px) {
            float wx = static_cast<float>(vp.pixel_to_real(px));
            float n  = fbm(wx, wy, perm); // in [-1, 1]
            float t  = std::clamp(n * 0.5f + 0.5f, 0.0f, 1.0f);
            buf.set(px, py, palette.sample(t));
        }
    }
}

} // namespace artgen

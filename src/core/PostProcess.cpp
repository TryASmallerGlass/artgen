#include "artgen/PostProcess.h"
#include "artgen/Color.h"
#include <algorithm>
#include <cmath>

namespace artgen {

static float clamp01(float v) { return std::clamp(v, 0.0f, 1.0f); }

void PostProcessor::apply(PixelBuffer& buf, const PostProcessParams& p) {
    if (!p.enabled()) return;

    const int W = buf.width(), H = buf.height();
    const float cx = W * 0.5f, cy = H * 0.5f;
    const float max_dist = std::sqrt(cx * cx + cy * cy);
    const float inv_max  = (max_dist > 1e-4f) ? 1.0f / max_dist : 1.0f;
    const float inv_gamma = (p.gamma > 1e-4f) ? 1.0f / p.gamma : 1.0f;

    for (int y = 0; y < H; ++y) {
        for (int x = 0; x < W; ++x) {
            RGBA c = buf.get(x, y);

            // Gamma correction
            if (p.gamma != 1.0f) {
                c.r = std::pow(clamp01(c.r), inv_gamma);
                c.g = std::pow(clamp01(c.g), inv_gamma);
                c.b = std::pow(clamp01(c.b), inv_gamma);
            }

            // Brightness (additive)
            if (p.brightness != 0.0f) {
                c.r = clamp01(c.r + p.brightness);
                c.g = clamp01(c.g + p.brightness);
                c.b = clamp01(c.b + p.brightness);
            }

            // Contrast: (v - 0.5) * contrast + 0.5
            if (p.contrast != 1.0f) {
                c.r = clamp01((c.r - 0.5f) * p.contrast + 0.5f);
                c.g = clamp01((c.g - 0.5f) * p.contrast + 0.5f);
                c.b = clamp01((c.b - 0.5f) * p.contrast + 0.5f);
            }

            // Saturation: lerp each channel toward luminance
            if (p.saturation != 1.0f) {
                float lum = 0.2126f * c.r + 0.7152f * c.g + 0.0722f * c.b;
                c.r = clamp01(lum + (c.r - lum) * p.saturation);
                c.g = clamp01(lum + (c.g - lum) * p.saturation);
                c.b = clamp01(lum + (c.b - lum) * p.saturation);
            }

            // Vignette: smooth radial darkening (quadratic falloff)
            if (p.vignette > 0.0f) {
                float dx = x - cx, dy = y - cy;
                float dist_norm = std::sqrt(dx * dx + dy * dy) * inv_max;
                float vf = clamp01(1.0f - p.vignette * dist_norm * dist_norm);
                c.r *= vf; c.g *= vf; c.b *= vf;
            }

            buf.set(x, y, c);
        }
    }
}

} // namespace artgen

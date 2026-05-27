#pragma once
#include "artgen/PixelBuffer.h"

namespace artgen {

struct PostProcessParams {
    float gamma      = 1.0f;   // 1.0 = off; >1 brightens, <1 darkens
    float contrast   = 1.0f;   // 1.0 = no change
    float brightness = 0.0f;   // additive offset, 0.0 = no change
    float saturation = 1.0f;   // 1.0 = no change, 0.0 = grayscale
    float vignette   = 0.0f;   // 0.0 = off, 1.0 = strong radial darkening

    bool enabled() const {
        return gamma      != 1.0f || contrast  != 1.0f ||
               brightness != 0.0f || saturation != 1.0f ||
               vignette   != 0.0f;
    }
};

class PostProcessor {
public:
    static void apply(PixelBuffer& buf, const PostProcessParams& params);
};

} // namespace artgen

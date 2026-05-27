#pragma once
#include "PixelBuffer.h"
#include "Viewport.h"

namespace artgen {

class IAlgorithm {
public:
    virtual ~IAlgorithm() = default;
    virtual void        render(PixelBuffer& buf, const Viewport& vp) const = 0;
    virtual const char* name() const = 0;
    // Return false for algorithms with global spatial state (e.g. reaction-diffusion)
    // that cannot be correctly divided into independent tiles.
    virtual bool        is_tileable() const { return true; }
    // Cyclic phase offset [0,1) applied to palette sampling; no-op by default.
    virtual void        set_palette_phase(float) {}
};

} // namespace artgen

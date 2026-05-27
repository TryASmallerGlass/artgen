#pragma once
#include "Color.h"
#include <vector>

namespace artgen {

struct GradientStop {
    float position; // [0, 1]
    RGBA  color;
};

class Palette {
public:
    bool  use_lab_interpolation = false;
    float phase                 = 0.0f;  // cyclic offset applied before sampling

    void add_stop(float position, RGBA color);

    // Sample at t in [0, 1] with phase offset applied (wraps modulo 1).
    RGBA sample(float t) const;

    // Built-in palettes
    static Palette classic_mandelbrot();
    static Palette fire();
    static Palette ice();
    static Palette electric();
    static Palette grayscale();

private:
    std::vector<GradientStop> m_stops;

    RGBA sample_linear(float t) const;
    RGBA sample_lab(float t) const;
};

} // namespace artgen

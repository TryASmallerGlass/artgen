#pragma once

namespace artgen {

struct Viewport {
    double real_min = -2.5;
    double real_max =  1.0;
    double imag_min = -1.25;
    double imag_max =  1.25;
    int    pixel_width  = 1920;
    int    pixel_height = 1080;

    double pixel_to_real(int x) const {
        if (pixel_width <= 1) return (real_min + real_max) * 0.5;
        return real_min + (real_max - real_min) * x / (pixel_width - 1);
    }

    double pixel_to_imag(int y) const {
        if (pixel_height <= 1) return (imag_min + imag_max) * 0.5;
        return imag_min + (imag_max - imag_min) * y / (pixel_height - 1);
    }
};

} // namespace artgen

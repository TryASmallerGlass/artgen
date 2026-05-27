#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Color.h"
#include <string>
#include <vector>

namespace artgen {

struct LSystemRule {
    char        predecessor;
    std::string successor;
};

class LSystemAlgorithm : public IAlgorithm {
public:
    std::string              axiom      = "F";
    std::vector<LSystemRule> rules;
    int                      iterations = 5;
    float                    angle_deg  = 25.7f;
    RGBA                     fg_color   = {0.2f, 0.8f, 0.2f, 1.0f};
    RGBA                     bg_color   = {0.03f, 0.05f, 0.03f, 1.0f};

    const char* name()        const override { return "lsystem"; }
    bool        is_tileable() const override { return false; }
    void        render(PixelBuffer& buf, const Viewport& vp) const override;

    // Named presets: "plant" "dragon" "sierpinski" "hilbert" "tree"
    void set_preset(const std::string& name);

private:
    std::string expand() const;

    // Run the turtle interpreter. do_draw=false → just find bounding box.
    void turtle_pass(const std::string& cmds, float angle_rad, float step,
                     bool do_draw, PixelBuffer* buf,
                     float start_x, float start_y,
                     float& mn_x, float& mn_y,
                     float& mx_x, float& mx_y) const;

    void draw_line(PixelBuffer& buf,
                   float x0, float y0, float x1, float y1) const;
};

} // namespace artgen

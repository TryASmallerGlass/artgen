#include "artgen/algorithms/LSystem.h"
#include <algorithm>
#include <cmath>
#include <stack>

namespace artgen {

static constexpr float kPi = 3.14159265358979f;

void LSystemAlgorithm::set_preset(const std::string& name) {
    rules.clear();
    if (name == "plant") {
        axiom = "X";
        rules = {{'X', "F+[[X]-X]-F[-FX]+X"}, {'F', "FF"}};
        iterations = 6;
        angle_deg  = 25.0f;
    } else if (name == "dragon") {
        axiom = "FX";
        rules = {{'X', "X+YF+"}, {'Y', "-FX-Y"}};
        iterations = 12;
        angle_deg  = 90.0f;
    } else if (name == "sierpinski") {
        axiom = "F-G-G";
        rules = {{'F', "F-G+F+G-F"}, {'G', "GG"}};
        iterations = 6;
        angle_deg  = 120.0f;
    } else if (name == "hilbert") {
        axiom = "A";
        rules = {{'A', "+BF-AFA-FB+"}, {'B', "-AF+BFB+FA-"}};
        iterations = 6;
        angle_deg  = 90.0f;
    } else if (name == "tree") {
        axiom = "F";
        rules = {{'F', "FF+[+F-F-F]-[-F+F+F]"}};
        iterations = 4;
        angle_deg  = 22.5f;
    }
}

std::string LSystemAlgorithm::expand() const {
    std::string current = axiom;
    for (int i = 0; i < iterations; ++i) {
        std::string next;
        next.reserve(current.size() * 4);
        for (char c : current) {
            bool replaced = false;
            for (const auto& r : rules) {
                if (r.predecessor == c) { next += r.successor; replaced = true; break; }
            }
            if (!replaced) next += c;
        }
        current = std::move(next);
    }
    return current;
}

void LSystemAlgorithm::turtle_pass(
    const std::string& cmds, float angle_rad, float step,
    bool do_draw, PixelBuffer* buf,
    float start_x, float start_y,
    float& mn_x, float& mn_y,
    float& mx_x, float& mx_y) const
{
    struct State { float x, y, a; };
    std::stack<State> stk;

    // Start heading upward (-Y direction in screen space)
    float x = start_x, y = start_y, a = -kPi * 0.5f;
    mn_x = mx_x = x;
    mn_y = mx_y = y;

    auto track = [&]() {
        if (x < mn_x) mn_x = x; if (x > mx_x) mx_x = x;
        if (y < mn_y) mn_y = y; if (y > mx_y) mx_y = y;
    };

    for (char c : cmds) {
        switch (c) {
        case 'F': case 'G': {
            float nx = x + step * std::cos(a);
            float ny = y + step * std::sin(a);
            if (do_draw && buf) draw_line(*buf, x, y, nx, ny);
            x = nx; y = ny; track();
            break;
        }
        case 'f':
            x += step * std::cos(a);
            y += step * std::sin(a);
            track();
            break;
        case '+': a -= angle_rad; break;   // left turn (counterclockwise)
        case '-': a += angle_rad; break;   // right turn (clockwise)
        case '|': a += kPi; break;         // U-turn
        case '[': stk.push({x, y, a}); break;
        case ']':
            if (!stk.empty()) { auto s = stk.top(); stk.pop(); x=s.x; y=s.y; a=s.a; }
            break;
        default: break;
        }
    }
}

void LSystemAlgorithm::draw_line(PixelBuffer& buf,
    float x0, float y0, float x1, float y1) const
{
    int W = buf.width(), H = buf.height();
    int ix0 = static_cast<int>(x0), iy0 = static_cast<int>(y0);
    int ix1 = static_cast<int>(x1), iy1 = static_cast<int>(y1);

    // Bresenham's line
    int dx = std::abs(ix1 - ix0), sx = (ix0 < ix1) ? 1 : -1;
    int dy = std::abs(iy1 - iy0), sy = (iy0 < iy1) ? 1 : -1;
    int err = dx - dy;

    for (;;) {
        if (ix0 >= 0 && ix0 < W && iy0 >= 0 && iy0 < H)
            buf.set(ix0, iy0, fg_color);
        if (ix0 == ix1 && iy0 == iy1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; ix0 += sx; }
        if (e2 <  dx) { err += dx; iy0 += sy; }
    }
}

void LSystemAlgorithm::render(PixelBuffer& buf, const Viewport& /*vp*/) const {
    const int W = buf.width();
    const int H = buf.height();

    buf.fill(bg_color);

    std::string cmds = expand();
    float angle_rad = angle_deg * kPi / 180.0f;

    // First pass: unit-step from origin to find bounding box
    float mn_x, mn_y, mx_x, mx_y;
    turtle_pass(cmds, angle_rad, 1.0f, false, nullptr,
                0.0f, 0.0f, mn_x, mn_y, mx_x, mx_y);

    float span_x = mx_x - mn_x;
    float span_y = mx_y - mn_y;
    if (span_x < 1e-6f) span_x = 1.0f;
    if (span_y < 1e-6f) span_y = 1.0f;

    // Scale to fill 90% of the viewport, centered
    float scale = std::min(W * 0.90f / span_x, H * 0.90f / span_y);
    float ox = (W - span_x * scale) * 0.5f - mn_x * scale;
    float oy = (H - span_y * scale) * 0.5f - mn_y * scale;

    // Second pass: draw
    float d0, d1, d2, d3;
    turtle_pass(cmds, angle_rad, scale, true, &buf,
                ox, oy, d0, d1, d2, d3);
}

} // namespace artgen

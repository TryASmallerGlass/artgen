#include "artgen/Palette.h"
#include "artgen/Color.h"
#include <algorithm>
#include <cmath>

namespace artgen {

void Palette::add_stop(float position, RGBA color) {
    m_stops.push_back({position, color});
    std::sort(m_stops.begin(), m_stops.end(),
        [](const GradientStop& a, const GradientStop& b) {
            return a.position < b.position;
        });
}

RGBA Palette::sample(float t) const {
    // Apply cyclic phase offset; fmod can return negative so add 1 first
    if (phase != 0.0f)
        t = std::fmod(t + phase + 1.0f, 1.0f);
    return use_lab_interpolation ? sample_lab(t) : sample_linear(t);
}

RGBA Palette::sample_linear(float t) const {
    if (m_stops.empty()) return {0.0f, 0.0f, 0.0f, 1.0f};
    if (t <= m_stops.front().position) return m_stops.front().color;
    if (t >= m_stops.back().position)  return m_stops.back().color;
    for (size_t i = 1; i < m_stops.size(); ++i) {
        if (t <= m_stops[i].position) {
            float lo = m_stops[i-1].position;
            float hi = m_stops[i].position;
            float u  = (t - lo) / (hi - lo);
            return lerp(m_stops[i-1].color, m_stops[i].color, u);
        }
    }
    return m_stops.back().color;
}

RGBA Palette::sample_lab(float t) const {
    if (m_stops.empty()) return {0.0f, 0.0f, 0.0f, 1.0f};
    if (t <= m_stops.front().position) return m_stops.front().color;
    if (t >= m_stops.back().position)  return m_stops.back().color;
    for (size_t i = 1; i < m_stops.size(); ++i) {
        if (t <= m_stops[i].position) {
            float lo = m_stops[i-1].position;
            float hi = m_stops[i].position;
            float u  = (t - lo) / (hi - lo);

            const RGBA& ca = m_stops[i-1].color;
            const RGBA& cb = m_stops[i].color;

            LAB la = rgb_to_lab({ca.r, ca.g, ca.b});
            LAB lb = rgb_to_lab({cb.r, cb.g, cb.b});

            LAB lm{la.l + (lb.l - la.l) * u,
                   la.a + (lb.a - la.a) * u,
                   la.b + (lb.b - la.b) * u};

            RGB rm  = lab_to_rgb(lm);
            float a = ca.a + (cb.a - ca.a) * u;
            return {rm.r, rm.g, rm.b, a};
        }
    }
    return m_stops.back().color;
}

Palette Palette::classic_mandelbrot() {
    Palette p;
    p.add_stop(0.00f, {0.000f, 0.000f, 0.000f, 1.0f});
    p.add_stop(0.02f, {0.125f, 0.420f, 0.796f, 1.0f});
    p.add_stop(0.25f, {0.929f, 1.000f, 1.000f, 1.0f});
    p.add_stop(0.50f, {1.000f, 0.667f, 0.000f, 1.0f});
    p.add_stop(0.75f, {0.000f, 0.008f, 0.000f, 1.0f});
    p.add_stop(1.00f, {0.000f, 0.000f, 0.000f, 1.0f});
    return p;
}

Palette Palette::fire() {
    Palette p;
    p.add_stop(0.00f, {0.0f, 0.0f, 0.0f, 1.0f});
    p.add_stop(0.33f, {0.8f, 0.0f, 0.0f, 1.0f});
    p.add_stop(0.66f, {1.0f, 0.5f, 0.0f, 1.0f});
    p.add_stop(1.00f, {1.0f, 1.0f, 0.8f, 1.0f});
    return p;
}

Palette Palette::ice() {
    Palette p;
    p.add_stop(0.00f, {0.00f, 0.00f, 0.10f, 1.0f});
    p.add_stop(0.50f, {0.00f, 0.50f, 0.80f, 1.0f});
    p.add_stop(1.00f, {0.90f, 0.95f, 1.00f, 1.0f});
    return p;
}

Palette Palette::electric() {
    Palette p;
    p.add_stop(0.00f, {0.0f, 0.0f, 0.0f, 1.0f});
    p.add_stop(0.25f, {0.3f, 0.0f, 0.6f, 1.0f});
    p.add_stop(0.50f, {0.0f, 0.0f, 1.0f, 1.0f});
    p.add_stop(0.75f, {0.0f, 0.8f, 1.0f, 1.0f});
    p.add_stop(1.00f, {1.0f, 1.0f, 1.0f, 1.0f});
    return p;
}

Palette Palette::grayscale() {
    Palette p;
    p.add_stop(0.0f, {0.0f, 0.0f, 0.0f, 1.0f});
    p.add_stop(1.0f, {1.0f, 1.0f, 1.0f, 1.0f});
    return p;
}

} // namespace artgen

#include "artgen/Color.h"
#include <algorithm>
#include <cmath>

namespace artgen {

// ── HSL ──────────────────────────────────────────────────────────────────────

static float hue2rgb(float p, float q, float t) {
    if (t < 0.0f) t += 1.0f;
    if (t > 1.0f) t -= 1.0f;
    if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
    if (t < 1.0f/2.0f) return q;
    if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
    return p;
}

RGB hsl_to_rgb(HSL hsl) {
    float h = hsl.h / 360.0f, s = hsl.s, l = hsl.l;
    if (s == 0.0f) return {l, l, l};
    float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
    float p = 2.0f * l - q;
    return {hue2rgb(p, q, h + 1.0f/3.0f),
            hue2rgb(p, q, h),
            hue2rgb(p, q, h - 1.0f/3.0f)};
}

HSL rgb_to_hsl(RGB rgb) {
    float mx = std::max({rgb.r, rgb.g, rgb.b});
    float mn = std::min({rgb.r, rgb.g, rgb.b});
    float l  = (mx + mn) * 0.5f;
    if (mx == mn) return {0.0f, 0.0f, l};
    float d = mx - mn;
    float s = l > 0.5f ? d / (2.0f - mx - mn) : d / (mx + mn);
    float h = 0.0f;
    if      (mx == rgb.r) h = (rgb.g - rgb.b) / d + (rgb.g < rgb.b ? 6.0f : 0.0f);
    else if (mx == rgb.g) h = (rgb.b - rgb.r) / d + 2.0f;
    else                  h = (rgb.r - rgb.g) / d + 4.0f;
    return {h * 60.0f, s, l};
}

// ── HSV ──────────────────────────────────────────────────────────────────────

RGB hsv_to_rgb(HSV hsv) {
    float h = hsv.h / 60.0f, s = hsv.s, v = hsv.v;
    int   i = static_cast<int>(h) % 6;
    float f = h - static_cast<int>(h);
    float p = v * (1.0f - s);
    float q = v * (1.0f - f * s);
    float t = v * (1.0f - (1.0f - f) * s);
    switch (i) {
        case 0: return {v, t, p};
        case 1: return {q, v, p};
        case 2: return {p, v, t};
        case 3: return {p, q, v};
        case 4: return {t, p, v};
        default: return {v, p, q};
    }
}

HSV rgb_to_hsv(RGB rgb) {
    float mx = std::max({rgb.r, rgb.g, rgb.b});
    float mn = std::min({rgb.r, rgb.g, rgb.b});
    float d  = mx - mn;
    float h  = 0.0f;
    if (d != 0.0f) {
        if      (mx == rgb.r) h = (rgb.g - rgb.b) / d + (rgb.g < rgb.b ? 6.0f : 0.0f);
        else if (mx == rgb.g) h = (rgb.b - rgb.r) / d + 2.0f;
        else                  h = (rgb.r - rgb.g) / d + 4.0f;
        h *= 60.0f;
    }
    return {h, mx == 0.0f ? 0.0f : d / mx, mx};
}

// ── sRGB <-> linear ──────────────────────────────────────────────────────────

static float linearize(float c) {
    return c <= 0.04045f ? c / 12.92f
                         : std::pow((c + 0.055f) / 1.055f, 2.4f);
}

static float delinearize(float c) {
    return c <= 0.0031308f ? c * 12.92f
                           : 1.055f * std::pow(c, 1.0f / 2.4f) - 0.055f;
}

// ── XYZ (D65) ────────────────────────────────────────────────────────────────

XYZ rgb_to_xyz(RGB rgb) {
    float r = linearize(rgb.r), g = linearize(rgb.g), b = linearize(rgb.b);
    return {r * 0.4124564f + g * 0.3575761f + b * 0.1804375f,
            r * 0.2126729f + g * 0.7151522f + b * 0.0721750f,
            r * 0.0193339f + g * 0.1191920f + b * 0.9503041f};
}

RGB xyz_to_rgb(XYZ xyz) {
    float r = xyz.x *  3.2404542f + xyz.y * -1.5371385f + xyz.z * -0.4985314f;
    float g = xyz.x * -0.9692660f + xyz.y *  1.8760108f + xyz.z *  0.0415560f;
    float b = xyz.x *  0.0556434f + xyz.y * -0.2040259f + xyz.z *  1.0572252f;
    return {std::clamp(delinearize(r), 0.0f, 1.0f),
            std::clamp(delinearize(g), 0.0f, 1.0f),
            std::clamp(delinearize(b), 0.0f, 1.0f)};
}

// ── LAB (D65 white point) ────────────────────────────────────────────────────

static constexpr float D65_X = 0.95047f;
static constexpr float D65_Y = 1.00000f;
static constexpr float D65_Z = 1.08883f;

static float lab_f(float t) {
    constexpr float d = 6.0f / 29.0f;
    return t > d * d * d ? std::cbrt(t) : t / (3.0f * d * d) + 4.0f / 29.0f;
}

static float lab_f_inv(float t) {
    constexpr float d = 6.0f / 29.0f;
    return t > d ? t * t * t : 3.0f * d * d * (t - 4.0f / 29.0f);
}

LAB xyz_to_lab(XYZ xyz) {
    float fx = lab_f(xyz.x / D65_X);
    float fy = lab_f(xyz.y / D65_Y);
    float fz = lab_f(xyz.z / D65_Z);
    return {116.0f * fy - 16.0f, 500.0f * (fx - fy), 200.0f * (fy - fz)};
}

XYZ lab_to_xyz(LAB lab) {
    float fy = (lab.l + 16.0f) / 116.0f;
    return {D65_X * lab_f_inv(fy + lab.a / 500.0f),
            D65_Y * lab_f_inv(fy),
            D65_Z * lab_f_inv(fy - lab.b / 200.0f)};
}

LAB rgb_to_lab(RGB rgb) { return xyz_to_lab(rgb_to_xyz(rgb)); }
RGB lab_to_rgb(LAB lab) { return xyz_to_rgb(lab_to_xyz(lab)); }

// ── CMYK ─────────────────────────────────────────────────────────────────────

CMYK rgb_to_cmyk(RGB rgb) {
    float k = 1.0f - std::max({rgb.r, rgb.g, rgb.b});
    if (k >= 1.0f) return {0.0f, 0.0f, 0.0f, 1.0f};
    float inv = 1.0f / (1.0f - k);
    return {(1.0f - rgb.r - k) * inv,
            (1.0f - rgb.g - k) * inv,
            (1.0f - rgb.b - k) * inv, k};
}

RGB cmyk_to_rgb(CMYK c) {
    float mk = 1.0f - c.k;
    return {(1.0f - c.c) * mk,
            (1.0f - c.m) * mk,
            (1.0f - c.y) * mk};
}

// ── Utilities ────────────────────────────────────────────────────────────────

uint8_t  to_u8(float v)  { return static_cast<uint8_t> (std::clamp(v, 0.0f, 1.0f) * 255.0f   + 0.5f); }
uint16_t to_u16(float v) { return static_cast<uint16_t>(std::clamp(v, 0.0f, 1.0f) * 65535.0f + 0.5f); }

RGBA lerp(RGBA a, RGBA b, float t) {
    return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t,
            a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t};
}

float delta_e(LAB a, LAB b) {
    float dl = a.l - b.l, da = a.a - b.a, db = a.b - b.b;
    return std::sqrt(dl*dl + da*da + db*db);
}

} // namespace artgen

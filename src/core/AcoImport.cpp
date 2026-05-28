#include "artgen/AcoImport.h"
#include "artgen/Color.h"
#include <fstream>
#include <stdexcept>
#include <cstdint>
#include <vector>

namespace artgen {

// ── Big-endian read helpers ───────────────────────────────────────────────────

static uint16_t read_u16_be(std::istream& s) {
    uint8_t b[2];
    s.read(reinterpret_cast<char*>(b), 2);
    return static_cast<uint16_t>((b[0] << 8) | b[1]);
}

static void skip_utf16_name(std::istream& s) {
    /* uint16_t zero = */ read_u16_be(s);   // always 0 in v2
    uint16_t len = read_u16_be(s);
    // skip len UTF-16BE characters
    for (uint16_t i = 0; i < len; ++i) read_u16_be(s);
}

// ── Colour space conversion ───────────────────────────────────────────────────

static RGBA swatch_to_rgba(uint16_t space,
                            uint16_t c0, uint16_t c1,
                            uint16_t c2, uint16_t /*c3*/) {
    auto u = [](uint16_t v) { return v / 65535.0f; };

    switch (space) {
    case 0: // RGB  — components are 0–65535
        return {u(c0), u(c1), u(c2), 1.0f};

    case 1: // HSB (HSV) — H: 0–65535 ≙ 0–360°; S,B: 0–65535 ≙ 0–100%
        {
            float h = c0 / 65535.0f * 360.0f;
            float s = c1 / 65535.0f;
            float v = c2 / 65535.0f;
            RGB rgb = hsv_to_rgb({h, s, v});
            return {rgb.r, rgb.g, rgb.b, 1.0f};
        }

    case 2: // CMYK — 0 = 100%, 65535 = 0% (inverted)
        {
            float c = 1.0f - u(c0);
            float m = 1.0f - u(c1);
            float y = 1.0f - u(c2);
            // c3 unused here; treat K=0 to get the pure CMY representation
            RGB rgb = cmyk_to_rgb({c, m, y, 0.0f});
            return {rgb.r, rgb.g, rgb.b, 1.0f};
        }

    case 7: // Lab — L: 0–10000 ≙ 0–100; a,b: 0–65535 ≙ -128..127
        {
            float L  = c0 / 100.0f;
            float a  = static_cast<int16_t>(c1) / 100.0f;
            float b  = static_cast<int16_t>(c2) / 100.0f;
            RGB rgb  = lab_to_rgb({L, a, b});
            return {rgb.r, rgb.g, rgb.b, 1.0f};
        }

    case 8: // Grayscale — 0–10000 ≙ 0–100%
        {
            float g = c0 / 10000.0f;
            return {g, g, g, 1.0f};
        }

    default: // Unknown — return mid-gray
        return {0.5f, 0.5f, 0.5f, 1.0f};
    }
}

// ── Reader ────────────────────────────────────────────────────────────────────

struct Swatch { RGBA color; };

static std::vector<Swatch> read_block(std::istream& s, bool has_names) {
    uint16_t count = read_u16_be(s);
    std::vector<Swatch> out;
    out.reserve(count);
    for (uint16_t i = 0; i < count && s.good(); ++i) {
        uint16_t space = read_u16_be(s);
        uint16_t c0    = read_u16_be(s);
        uint16_t c1    = read_u16_be(s);
        uint16_t c2    = read_u16_be(s);
        uint16_t c3    = read_u16_be(s);
        if (has_names) skip_utf16_name(s);
        out.push_back({swatch_to_rgba(space, c0, c1, c2, c3)});
    }
    return out;
}

// ── Public API ────────────────────────────────────────────────────────────────

Palette load_aco(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open())
        throw std::runtime_error("Cannot open ACO file: " + path);

    uint16_t version = read_u16_be(f);
    if (!f.good())
        throw std::runtime_error("Malformed ACO file (header): " + path);

    std::vector<Swatch> swatches;

    if (version == 1) {
        swatches = read_block(f, false);
    } else if (version == 2) {
        // V2 files may also start with a V1 block; skip it and read V2 block
        uint16_t count1 = read_u16_be(f);
        // Skip count1 V1 swatches (5 uint16 each)
        f.seekg(static_cast<std::streamoff>(count1) * 10, std::ios::cur);
        if (!f.good())
            throw std::runtime_error("Malformed ACO file (V1 block): " + path);
        // Read V2 header
        uint16_t v2 = read_u16_be(f);
        if (v2 != 2)
            throw std::runtime_error("Unexpected ACO version in second block: " + path);
        swatches = read_block(f, true);
    } else {
        throw std::runtime_error("Unsupported ACO version " +
                                  std::to_string(version) + ": " + path);
    }

    if (swatches.empty())
        throw std::runtime_error("ACO file contains no swatches: " + path);

    Palette pal;
    if (swatches.size() == 1) {
        pal.add_stop(0.0f, swatches[0].color);
        pal.add_stop(1.0f, swatches[0].color);
    } else {
        float step = 1.0f / static_cast<float>(swatches.size() - 1);
        for (size_t i = 0; i < swatches.size(); ++i)
            pal.add_stop(i * step, swatches[i].color);
    }
    return pal;
}

} // namespace artgen

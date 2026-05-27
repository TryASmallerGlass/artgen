#include "artgen/PixelBuffer.h"
#include <stdexcept>

namespace artgen {

PixelBuffer::PixelBuffer(int width, int height, Depth depth)
    : m_width(width), m_height(height), m_depth(depth)
{
    if (width <= 0 || height <= 0)
        throw std::invalid_argument("PixelBuffer: dimensions must be positive");
    size_t n = static_cast<size_t>(width) * static_cast<size_t>(height) * 4;
    if (depth == Depth::U8)  m_u8.resize(n, 0);
    else                     m_u16.resize(n, 0);
}

RGBA PixelBuffer::get(int x, int y) const {
    int i = index(x, y);
    if (m_depth == Depth::U8)
        return {m_u8[i]   / 255.0f, m_u8[i+1] / 255.0f,
                m_u8[i+2] / 255.0f, m_u8[i+3] / 255.0f};
    return {m_u16[i]   / 65535.0f, m_u16[i+1] / 65535.0f,
            m_u16[i+2] / 65535.0f, m_u16[i+3] / 65535.0f};
}

void PixelBuffer::set(int x, int y, RGBA c) {
    int i = index(x, y);
    if (m_depth == Depth::U8) {
        m_u8[i]   = to_u8(c.r);
        m_u8[i+1] = to_u8(c.g);
        m_u8[i+2] = to_u8(c.b);
        m_u8[i+3] = to_u8(c.a);
    } else {
        m_u16[i]   = to_u16(c.r);
        m_u16[i+1] = to_u16(c.g);
        m_u16[i+2] = to_u16(c.b);
        m_u16[i+3] = to_u16(c.a);
    }
}

void PixelBuffer::fill(RGBA c) {
    for (int y = 0; y < m_height; ++y)
        for (int x = 0; x < m_width; ++x)
            set(x, y, c);
}

} // namespace artgen

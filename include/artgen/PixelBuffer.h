#pragma once
#include "Color.h"
#include <vector>
#include <cstdint>

namespace artgen {

class PixelBuffer {
public:
    enum class Depth { U8, U16 };

    PixelBuffer(int width, int height, Depth depth = Depth::U8);

    int   width()  const { return m_width; }
    int   height() const { return m_height; }
    Depth depth()  const { return m_depth; }

    RGBA get(int x, int y) const;
    void set(int x, int y, RGBA color);
    void fill(RGBA color);

    const uint8_t*  data_u8()  const { return m_u8.data(); }
    const uint16_t* data_u16() const { return m_u16.data(); }

private:
    int   m_width, m_height;
    Depth m_depth;
    std::vector<uint8_t>  m_u8;
    std::vector<uint16_t> m_u16;

    int index(int x, int y) const { return (y * m_width + x) * 4; }
};

} // namespace artgen

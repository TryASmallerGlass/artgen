#pragma once
#include <cstdint>

namespace artgen {

struct RGB  { float r, g, b;       };
struct RGBA { float r, g, b, a;    };
struct HSL  { float h, s, l;       }; // h: [0,360), s/l: [0,1]
struct HSV  { float h, s, v;       }; // h: [0,360), s/v: [0,1]
struct XYZ  { float x, y, z;       };
struct LAB  { float l, a, b;       }; // L: [0,100], a/b: [-128,127]
struct CMYK { float c, m, y, k;    }; // all [0,1]

RGB     hsl_to_rgb(HSL hsl);
HSL     rgb_to_hsl(RGB rgb);
RGB     hsv_to_rgb(HSV hsv);
HSV     rgb_to_hsv(RGB rgb);
XYZ     rgb_to_xyz(RGB rgb);
RGB     xyz_to_rgb(XYZ xyz);
LAB     xyz_to_lab(XYZ xyz);
XYZ     lab_to_xyz(LAB lab);
LAB     rgb_to_lab(RGB rgb);
RGB     lab_to_rgb(LAB lab);
CMYK    rgb_to_cmyk(RGB rgb);
RGB     cmyk_to_rgb(CMYK cmyk);

uint8_t  to_u8(float v);
uint16_t to_u16(float v);
RGBA     lerp(RGBA a, RGBA b, float t);
float    delta_e(LAB a, LAB b); // CIE76

} // namespace artgen

#pragma once
#include "artgen/Palette.h"
#include <string>

namespace artgen {

// Load an Adobe Color Swatch (.aco) file and return it as a Palette.
//
// Supports color spaces: RGB, HSB, CMYK, LAB, Grayscale.
// Version 1 and Version 2 .aco files are both accepted.
// Gradient stops are distributed evenly from 0 to 1 across the swatches.
//
// Throws std::runtime_error on I/O failure or malformed data.
Palette load_aco(const std::string& path);

} // namespace artgen

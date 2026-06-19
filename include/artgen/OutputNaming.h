#pragma once
#include "artgen/config/SceneConfig.h"
#include "artgen/PixelBuffer.h"
#include <string>

namespace artgen {

// Returns an auto-generated output stem: "[algo7]_YYYYMMDD_HHMM_NNNNN".
// Increments the counter file in ImageSettings/.counter.
std::string make_output_stem(const std::string& algo_name);

// Serialises the full SceneConfig to ImageSettings/<stem>.json.
void save_settings_json(const SceneConfig& cfg, const std::string& stem);

// Samples the buffer and prints a warning to stderr if output is near-black or flat.
void check_render_quality(const PixelBuffer& buf, const std::string& algo_name);

} // namespace artgen

#pragma once
#include "artgen/IAlgorithm.h"
#include "artgen/Palette.h"
#include <cstdint>
#include <string>

namespace artgen {

enum class VoronoiMode   { Cells, Distance, Edge };
enum class VoronoiMetric { Euclidean, Manhattan, Chebyshev };

// Voronoi (Worley) diagram.
//
// `num_seeds` random seed points are scattered across the viewport.  Each pixel
// is coloured by the relationship between its distance to the nearest seed (d1)
// and second-nearest seed (d2):
//
//   "cells"    — hash of nearest-seed index  (each cell gets a distinct hue)
//   "distance" — d1 normalised to [0,1]      (gradient from seed outward)
//   "edge"     — 1 - (d2-d1)/(d2+d1)        (bright at Voronoi edges)
//
// Distance metrics: "euclidean", "manhattan", "chebyshev"
class VoronoiAlgorithm : public IAlgorithm {
public:
    int           num_seeds = 32;
    uint32_t      seed      = 42;
    VoronoiMode   mode      = VoronoiMode::Cells;
    VoronoiMetric metric    = VoronoiMetric::Euclidean;
    Palette       palette;

    VoronoiAlgorithm();
    const char* name() const override { return "voronoi"; }
    void render(PixelBuffer& buf, const Viewport& vp) const override;
    void set_palette_phase(float p) override { palette.phase = p; }
};

} // namespace artgen

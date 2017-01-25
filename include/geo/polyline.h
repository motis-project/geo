#pragma once

#include <vector>
#include <cassert>

#include "geo/latlng.h"

namespace geo {

using polyline = std::vector<latlng>;

double length(polyline const&);

polyline simplify(polyline const&, double const max_distance);

template <int TileSize, int MaxZoomLevel = 20>
polyline simplify(polyline const& line, uint32_t const z) {
  assert(z < MaxZoomLevel);

  struct look_up_table {
    constexpr look_up_table() : values() {
      constexpr auto kMinPixel = 1;
      for (auto i = 0; i <= MaxZoomLevel; ++i) {
        double shift = (1u << i) * TileSize;
        double b = shift / 2.0;
        double pixel_to_deg = 180. / b;
        double min_deg = kMinPixel * pixel_to_deg;
        values[i] = min_deg * min_deg;
      }
    }
    double values[MaxZoomLevel + 1];
  };
  constexpr auto lut = look_up_table{};

  return simplify(line, lut.values[z]);
}

polyline extract(polyline const&, size_t const from, size_t const to);

}  // namespace geo

#pragma once

#include <cassert>
#include <cinttypes>
#include <vector>

#include "geo/latlng.h"

namespace geo {

using polyline = std::vector<latlng>;

double length(polyline const&);

polyline simplify(polyline const&, double max_distance);

template <int TileSize, int MaxZoomLevel = 20>
polyline simplify(polyline const& line, uint32_t const z) {
  assert(z < MaxZoomLevel);

  struct look_up_table {
    constexpr look_up_table() : values_() {
      constexpr auto kMinPixel = 1;
      for (auto i = 0; i <= MaxZoomLevel; ++i) {
        double shift = (1U << i) * TileSize;
        double b = shift / 2.0;
        double pixel_to_deg = 180. / b;
        double min_deg = kMinPixel * pixel_to_deg;
        values_[i] = min_deg * min_deg;
      }
    }
    double values_[MaxZoomLevel + 1];
  };
  constexpr auto lut = look_up_table{};

  return simplify(line, lut.values[z]);
}

polyline extract(polyline const&, std::size_t from, std::size_t to);

inline std::vector<double> serialize(polyline const& p) {
  std::vector<double> result;
  result.resize(p.size() * 2);

  for (auto i = 0U; i < p.size(); ++i) {
    result[i * 2] = p[i].lat_;
    result[i * 2 + 1] = p[i].lng_;
  }

  return result;
}

template <typename Container>
inline polyline deserialize(Container const& container) {
  assert(container.size() % 2 == 0);
  polyline result;
  result.reserve(container.size() / 2);

  for (auto i = 0U; i < container.size(); i += 2) {
    result.emplace_back(container[i], container[i + 1]);
  }

  return result;
}

struct polyline_candidate {
  friend bool operator<(polyline_candidate const& a, polyline_candidate const& b) {
    return a.distance_to_polyline_ < b.distance_to_polyline_;
  }

  double distance_to_polyline_;
  geo::latlng best_;
  std::size_t segment_idx_;
};

polyline_candidate distance_to_polyline(latlng const&, polyline const&);

}  // namespace geo

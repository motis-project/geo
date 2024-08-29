#pragma once

#include <cassert>
#include <cinttypes>
#include <algorithm>
#include <ranges>
#include <vector>

#include "utl/pairwise.h"

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
  friend bool operator<(polyline_candidate const& a,
                        polyline_candidate const& b) {
    return a.distance_to_polyline_ < b.distance_to_polyline_;
  }

  double distance_to_polyline_;
  geo::latlng best_;
  std::size_t segment_idx_;
};

#if __cpp_lib_ranges_fold
template <std::ranges::range Polyline>
polyline_candidate distance_to_polyline(latlng const& x, Polyline&& c) {
  auto const segments = c | std::views::slide(2) | std::views::enumerate;
  return std::ranges::fold_left(
      segments,
      polyline_candidate{
          .distance_to_polyline_ = std::numeric_limits<double>::max(),
          .best_ = latlng{},
          .segment_idx_ = 0u},
      [&x](auto const best, auto const& current) {
        auto const& [index, segment] = current;
        auto const candidate = closest_on_segment(x, segment[0], segment[1]);
        auto const dist = distance(x, candidate);
        return (dist < best.distance_to_polyline_)
                   ? polyline_candidate{.distance_to_polyline_ = dist,
                                        .best_ = candidate,
                                        .segment_idx_ =
                                            static_cast<std::size_t>(index)}
                   : best;
      });
}
#else
template <typename Polyline>
polyline_candidate distance_to_polyline(latlng const& x, Polyline&& c) {
  auto min = std::numeric_limits<double>::max();
  auto best = latlng{};
  auto best_segment_idx = 0U;
  auto segment_idx = 0U;
  for (auto const [a, b] : utl::pairwise(c)) {
    auto const candidate = closest_on_segment(x, a, b);
    auto const dist = distance(x, candidate);
    if (dist < min) {
      min = dist;
      best = candidate;
      best_segment_idx = segment_idx;
    }
    ++segment_idx;
  }
  return {.distance_to_polyline_ = min,
          .best_ = best,
          .segment_idx_ = best_segment_idx};
}
#endif

}  // namespace geo

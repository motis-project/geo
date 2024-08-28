#include "geo/polyline.h"

#include "boost/geometry.hpp"

#include "utl/pairwise.h"

#include "geo/constants.h"

#include "geo/detail/register_latlng.h"
#include "geo/detail/register_polyline.h"

namespace geo {

double length(polyline const& p) {
  return boost::geometry::length(p) * kEarthRadiusMeters;
}

polyline simplify(polyline const& p, double const max_distance) {
  polyline result;
  boost::geometry::simplify(p, result, max_distance);
  return result;
}

polyline extract(polyline const& p, size_t const from, size_t const to) {
  geo::polyline result;
  result.reserve(std::abs(static_cast<int>(from) - static_cast<int>(to)) + 1);
  int const inc = (from < to) ? 1 : -1;
  for (int i = from; i != (static_cast<int>(to) + inc); i += inc) {
    result.push_back(p[i]);
  }
  return result;
}

polyline_candidate distance_to_polyline(latlng const& x, polyline const& c) {
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

}  // namespace geo

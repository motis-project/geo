#include "geo/polyline.h"

#include "boost/geometry.hpp"

#include "geo/constants.h"

#include "geo/detail/register_latlng.h"
#include "geo/detail/register_polyline.h"

namespace geo {

double length(polyline const& p) {
  return boost::geometry::length(p) * kEarthRadiusMeters;
}

polyline simplify(polyline const& p) {
  polyline result;
  boost::geometry::simplify(p, result, 0.0001);  // TODO constant -> zoom levels
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

}  // namespace geo

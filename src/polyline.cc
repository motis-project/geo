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

}  // namespace geo

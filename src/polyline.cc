#include "motis/geo/polyline.h"

#include "boost/geometry.hpp"

#include "motis/geo/constants.h"

#include "motis/geo/detail/register_latlng.h"
#include "motis/geo/detail/register_polygon.h"

namespace motis {
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
}  // namespace motis

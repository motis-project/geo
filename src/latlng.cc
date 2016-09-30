#include "motis/geo/latlng.h"

#include <cmath>

#include "boost/geometry.hpp"

#include "motis/geo/constants.h"
#include "motis/geo/detail/register_latlng.h"

namespace motis {
namespace geo {

double distance(latlng const& a, latlng const& b) {
  return boost::geometry::distance(a, b) * kEarthRadiusMeters;
}

// following non-public boost implementation
double bearing(latlng const& p1, latlng const& p2) {
  auto to_rad = [](auto const& deg) { return deg * kPI / 180.0; };
  auto to_deg = [](auto const& rad) { return rad * 180.0 / kPI; };

  double dlng = to_rad(p1.lng_) - to_rad(p2.lng_);
  double cos_p2lat = std::cos(to_rad(p2.lat_));

  auto bearing =
      std::atan2(std::sin(dlng) * cos_p2lat,
                 std::cos(to_rad(p1.lat_)) * std::sin(to_rad(p2.lat_)) -
                     std::sin(to_rad(p1.lat_)) * cos_p2lat * std::cos(dlng));

  return to_deg(std::fmod(bearing, 2 * kPI));
}

}  // namespace geo
}  // namespace motis

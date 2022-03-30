#include "geo/latlng.h"

#include <cmath>

#include "boost/geometry.hpp"

#include "geo/constants.h"
#include "geo/detail/register_latlng.h"
#include "geo/tile.h"
#include "geo/webmercator.h"

namespace geo {

std::ostream& operator<<(std::ostream& out, latlng const& pos) {
  return out << '(' << pos.lat_ << ", " << pos.lng_ << ")";
}

double distance(latlng const& a, latlng const& b) {
  return boost::geometry::distance(a, b) * kEarthRadiusMeters;
}

// following non-public boost implementation
double bearing(latlng const& p1, latlng const& p2) {
  auto to_rad = [](auto const& deg) { return deg * kPI / 180.0; };
  auto to_deg = [](auto const& rad) { return rad * 180.0 / kPI; };

  double dlng = to_rad(p1.lng_) - to_rad(p2.lng_);  // CCW from NORTH!
  double cos_p2lat = std::cos(to_rad(p2.lat_));

  auto bearing =
      std::atan2(std::sin(dlng) * cos_p2lat,
                 std::cos(to_rad(p1.lat_)) * std::sin(to_rad(p2.lat_)) -
                     std::sin(to_rad(p1.lat_)) * cos_p2lat * std::cos(dlng));

  return to_deg(std::fmod(bearing, 2 * kPI));
}

uint32_t tile_hash_32(latlng const& pos) {
  uint32_t hash = 0U;
  constexpr auto const kHashBits = sizeof(hash) * 8;
  constexpr auto const kZMax = kHashBits / 2;

  auto const merc = latlng_to_merc(pos);
  using proj = webmercator<1>;
  tile t{static_cast<uint32_t>(proj::merc_to_pixel_x(merc.x_, kZMax)),
         static_cast<uint32_t>(proj::merc_to_pixel_y(merc.y_, kZMax)),
         static_cast<uint32_t>(kZMax)};

  for (auto offset = 0U; offset < kHashBits; offset += 2) {
    assert(t.z_ != 0);

    auto quad_pos = t.quad_pos();
    hash = hash | quad_pos << offset;
    t = t.parent();
  }
  assert(t.z_ == 0);

  return hash;
}

}  // namespace geo

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
  double dlng = to_rad(p1.lng_) - to_rad(p2.lng_);  // CCW from NORTH!
  double cos_p2lat = std::cos(to_rad(p2.lat_));

  auto bearing =
      std::atan2(std::sin(dlng) * cos_p2lat,
                 std::cos(to_rad(p1.lat_)) * std::sin(to_rad(p2.lat_)) -
                     std::sin(to_rad(p1.lat_)) * cos_p2lat * std::cos(dlng));

  return to_deg(std::fmod(bearing, 2 * kPI));
}

// https://stackoverflow.com/a/4656937/10794188
latlng midpoint(latlng const& a, latlng const& b) {
  double d_lng = to_rad(b.lng_ - a.lng_);

  auto const a_lat = to_rad(a.lat_);
  auto const b_lat = to_rad(b.lat_);
  auto const a_lng = to_rad(a.lng_);

  auto const b_x = std::cos(b_lat) * std::cos(d_lng);
  auto const b_y = std::cos(b_lat) * std::sin(d_lng);

  auto const lat = std::atan2(
      std::sin(a_lat) + std::sin(b_lat),
      std::sqrt((std::cos(a_lat) + b_x) * (std::cos(a_lat) + b_x) + b_y * b_y));
  auto const lng = a_lng + std::atan2(b_y, std::cos(a_lat) + b_x);

  return {lat, lng};
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

latlng closest_on_segment(latlng const& x, latlng const& segment_from,
                          latlng const& segment_to) {
  auto const merc_x = latlng_to_merc(x);
  auto const merc_from = latlng_to_merc(segment_from);
  auto const merc_to = latlng_to_merc(segment_to);

  if (merc_x == merc_from || merc_x == merc_to) {
    return x;
  }

  auto const seg_dir = merc_to - merc_from;
  auto const seg_len = seg_dir.length();

  if (seg_len < kEpsilon) {
    return segment_from;
  }

  auto const start_vec = merc_x - merc_from;
  auto const end_vec = merc_x - merc_to;
  auto const start_angle =
      std::acos(seg_dir.dot(start_vec) / (seg_len * start_vec.length()));
  auto const end_angle =
      std::acos(seg_dir.dot(end_vec) / (seg_len * end_vec.length()));

  assert(!std::isnan(start_angle) && !std::isnan(end_angle));

  if (start_angle >= 90.0_rad) {
    return segment_from;
  } else if (end_angle <= 90.0_rad) {
    return segment_to;
  } else {
    // law of sines
    auto const beta = 90.0_rad - start_angle;
    auto const seg_offset = start_vec.length() * std::sin(beta);
    return merc_to_latlng(merc_from + seg_offset * seg_dir.normalize());
  }
}

}  // namespace geo

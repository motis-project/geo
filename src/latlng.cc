#include "geo/latlng.h"

#include <cmath>

#include "boost/geometry.hpp"

#include "geo/constants.h"
#include "geo/detail/register_latlng.h"
#include "geo/tile.h"
#include "geo/webmercator.h"

namespace geo {

constexpr static auto const kApproxDistanceLatDegrees =
    geo::kEarthRadiusMeters * geo::kPI / 180;

double lower_bound_distance_lng_degrees(latlng const& reference) {
  return std::clamp(1.0 - std::max(std::abs(reference.lat()),
                                   std::abs(reference.lat())) /
                              90.0,
                    0.0, 1.0) *
         kApproxDistanceLatDegrees;
}

double approx_distance_lng_degrees(latlng const& reference) {
  return geo::distance(
      reference, {reference.lat(),
                  reference.lng() + (reference.lng() < 0.0 ? 1.0 : -1.0)});
}

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

inline double get_angle(merc_xy const& v, merc_xy const& seg_dir,
                        double const seg_len) {
  auto const rel = seg_dir.dot(v) / (seg_len * v.length());
  if (rel >= 1 - kEpsilon) {
    return 0;
  } else if (rel <= -1 + kEpsilon) {
    return 180;
  }
  auto const angle = std::acos(rel);
  assert(!std::isnan(angle));
  return angle;
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
  auto const end_vec = merc_to - merc_x;

  auto const start_angle = get_angle(start_vec, seg_dir, seg_len);
  assert(!std::isnan(start_angle));
  if (start_angle >= to_rad(90.0)) {
    return segment_from;
  }
  auto const end_angle = get_angle(end_vec, seg_dir, seg_len);
  assert(!std::isnan(end_angle));
  if (end_angle >= to_rad(90.0)) {
    return segment_to;
  }

  // law of sines
  auto const beta = to_rad(90.0) - start_angle;
  auto const seg_offset = start_vec.length() * std::sin(beta);
  return merc_to_latlng(merc_from + seg_offset * seg_dir.normalize());
}

std::pair<latlng, double> approx_closest_on_segment(
    latlng const& x, latlng const& segment_from, latlng const& segment_to,
    double approx_distance_lng_degrees) {
  auto const to_approx_xy = [&](latlng const& ll, latlng const& ref) {
    auto const xdiff = ll.lng_ - ref.lng_;
    return xy{(xdiff > 180.0 ? (360.0 - std::abs(xdiff)) : xdiff) *
                  approx_distance_lng_degrees,
              (ll.lat_ - ref.lat_) * kApproxDistanceLatDegrees};
  };
  auto const squared_diff = [&](xy<double> const& a, xy<double> const& b) {
    return (a - b).dot(a - b);
  };
  auto const proj_x = to_approx_xy(x, x);
  auto const proj_from = to_approx_xy(segment_from, x);
  auto const proj_to = to_approx_xy(segment_to, x);

  if (proj_x == proj_from) {
    return {x, squared_diff(proj_x, proj_from)};
  }
  if (proj_x == proj_to) {
    return {x, squared_diff(proj_x, proj_to)};
  }

  auto const segment = proj_to - proj_from;
  auto const squared_len = segment.dot(segment);
  if (squared_len < kEpsilon) {
    return {segment_from, squared_diff(proj_x, proj_from)};
  }

  auto const dot_from = (proj_x - proj_from).dot(segment);
  if (dot_from < 0.0) {
    return {segment_from, squared_diff(proj_x, proj_from)};
  }
  auto const dot_to = (proj_x - proj_to).dot(proj_from - proj_to);
  if (dot_to < 0.0) {
    return {segment_to, squared_diff(proj_x, proj_to)};
  }

  auto const point_on_segment =
      (proj_from * dot_to + proj_to * dot_from) / squared_len;
  auto const xcoord =
      point_on_segment.x_ / approx_distance_lng_degrees + x.lng_;
  return {latlng{point_on_segment.y_ / kApproxDistanceLatDegrees + x.lat_,
                 xcoord > 180.0 ? xcoord - 360.0 : xcoord},  //
          squared_diff(proj_x, point_on_segment)};
}

// Destination point given distance and bearing from start point
// http://www.movable-type.co.uk/scripts/latlong.html
latlng destination_point(geo::latlng const& source, double const distance,
                         double const bearing) {
  // convert to rad
  auto const lat_source_rad = to_rad(source.lat_);
  auto const bearing_rad = to_rad(bearing);

  // reused
  auto const sin_lat_source = std::sin(lat_source_rad);
  auto const cos_lat_source = std::cos(lat_source_rad);
  auto const angular_distance = distance / kEarthRadiusMeters;
  auto const sin_angular_distance = std::sin(angular_distance);
  auto const cos_angular_distance = std::cos(angular_distance);

  // calculate lat/lon of destination
  auto const lat_dest =
      std::asin(sin_lat_source * cos_angular_distance +
                cos_lat_source * sin_angular_distance * std::cos(bearing_rad));
  auto const lon_dest =
      to_rad(source.lng_) +
      std::atan2(std::sin(bearing_rad) * sin_angular_distance * cos_lat_source,
                 cos_angular_distance - sin_lat_source * std::sin(lat_dest));

  // convert back to deg
  return geo::latlng{to_deg(lat_dest), to_deg(lon_dest)};
}

}  // namespace geo

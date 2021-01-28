#pragma once

#include <cmath>
#include <limits>

#include "geo/constants.h"
#include "geo/latlng.h"

// idea see:
// https://www.kaggle.com/c/santas-stolen-sleigh/forums/t/18049/simpler-faster-haversine-distance

namespace geo {

struct xyz {
  xyz() = default;
  explicit xyz(double x, double y, double z) : x_(x), y_(y), z_(z) {}
  explicit xyz(latlng const& ll) {
    auto const lat_rads = (kPI / 180.) * ll.lat_;
    auto const lng_rads = (kPI / 180.) * ll.lng_;

    x_ = 0.5 * std::cos(lat_rads) * std::sin(lng_rads);
    y_ = 0.5 * std::cos(lat_rads) * std::cos(lng_rads);
    z_ = 0.5 * std::sin(lat_rads);
  }

  double x_, y_, z_;
};

inline double haversine_distance(xyz const& a, xyz const& b) {
  auto const dx = a.x_ - b.x_;
  auto const dy = a.y_ - b.y_;
  auto const dz = a.z_ - b.z_;

  auto const r = std::sqrt(dx * dx + dy * dy + dz * dz);
  return 2 * kEarthRadiusMeters * std::asin(r);
}

inline double min_haversine_distance(xyz const& a, std::vector<xyz> const& bs) {
  auto d_min = std::numeric_limits<double>::infinity();
  for (auto const& b : bs) {
    auto const dx = a.x_ - b.x_;
    auto const dy = a.y_ - b.y_;
    auto const dz = a.z_ - b.z_;

    auto const d = dx * dx + dy * dy + dz * dz;
    if (d < d_min) {
      d_min = d;
    }
  }

  auto const r = std::sqrt(d_min);
  return 2 * kEarthRadiusMeters * std::asin(r);
}

}  // namespace geo

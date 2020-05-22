#pragma once

#include <cmath>

#include <algorithm>
#include <limits>

#include "geo/constants.h"
#include "geo/latlng.h"
#include "geo/polyline.h"

namespace geo {

struct box {
  box()
      : min_(std::numeric_limits<double>::infinity(),
             std::numeric_limits<double>::infinity()),
        max_(-std::numeric_limits<double>::infinity(),
             -std::numeric_limits<double>::infinity()) {}

  box(latlng min, latlng max) : min_(std::move(min)), max_(std::move(max)) {}

  explicit box(polyline const& line) : box{} { extend(line); }

  box(latlng const& center, double const dist_in_m) : box{center, center} {
    extend(dist_in_m);
  }

  void extend(polyline const& line) {
    for (auto const& pos : line) {
      extend(pos);
    }
  }

  void extend(box const& other) {
    extend(other.max_);
    extend(other.min_);
  }

  void extend(latlng const& pos) {
    max_.lat_ = std::max(max_.lat_, pos.lat_);
    max_.lng_ = std::max(max_.lng_, pos.lng_);

    min_.lat_ = std::min(min_.lat_, pos.lat_);
    min_.lng_ = std::min(min_.lng_, pos.lng_);
  }

  void extend(double dist_in_m) {
    // The distance of latitude degrees in km is always the same (~111000.0)
    double const d_lat = dist_in_m / 111000.0;

    min_.lat_ -= d_lat;
    max_.lat_ += d_lat;

    // The distance of longitude degrees depends on the latitude.
    double const min_lat_rad = min_.lat_ * (kPI / 180.0);
    double const min_m_per_deg = 111200.0 * std::cos(min_lat_rad);
    double const min_d_lng = std::abs(dist_in_m / min_m_per_deg);

    min_.lng_ -= min_d_lng;

    // The distance of longitude degrees depends on the latitude.
    double const max_lat_rad = max_.lat_ * (kPI / 180.0);
    double const max_m_per_deg = 111200.0 * std::cos(max_lat_rad);
    double const max_d_lng = std::abs(dist_in_m / max_m_per_deg);

    max_.lng_ += max_d_lng;
  }

  bool contains(latlng const& pos) const {
    return pos.lat_ > min_.lat_ && pos.lat_ < max_.lat_ &&
           pos.lng_ > min_.lng_ && pos.lng_ < max_.lng_;
  }

  latlng min_, max_;
};

inline geo::box make_box(std::initializer_list<geo::latlng> const coords) {
  geo::box b;
  for (auto const& c : coords) {
    b.extend(c);
  }
  return b;
}

}  // namespace geo

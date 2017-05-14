#pragma once

#include <limits>

#include "geo/latlng.h"
#include "geo/polyline.h"

namespace geo {

struct box {
  box()
      : min_(std::numeric_limits<double>::infinity(),
             std::numeric_limits<double>::infinity()),
        max_(-std::numeric_limits<double>::infinity(),
             -std::numeric_limits<double>::infinity()) {}

  explicit box(polyline const& line) : box{} { extend(line); }

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

  latlng min_, max_;
};

}  // namespace geo

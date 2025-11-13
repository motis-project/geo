#pragma once

#include <cinttypes>
#include <cmath>

#include "geo/latlng.h"

namespace geo {

struct fixed_latlng {
  static constexpr auto const kCoordinatePrecision = 1'000'0000;

  static std::int32_t double_to_fix(double const c) noexcept {
    return static_cast<int32_t>(std::round(c * kCoordinatePrecision));
  }
  static constexpr double fix_to_double(std::int32_t const c) noexcept {
    return static_cast<double>(c) / kCoordinatePrecision;
  }

  static fixed_latlng from_latlng(latlng const& x) {
    return {double_to_fix(x.lat()), double_to_fix(x.lng())};
  }

  double lat() const { return fix_to_double(lat_); }
  double lng() const { return fix_to_double(lng_); }

  operator latlng() const { return {lat(), lng()}; }

  std::int32_t lat_, lng_;
};

}  // namespace geo
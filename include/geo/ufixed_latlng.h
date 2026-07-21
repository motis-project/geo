#pragma once

#include <cinttypes>
#include <cmath>
#include <limits>

#include "libmorton/morton.h"

#include "geo/latlng.h"

namespace geo {

struct ufixed_latlng {
  static constexpr auto kCoordinatePrecision = 10'000'000U;
  static constexpr auto kUndefined =
      std::numeric_limits<std::uint32_t>::max();
  static constexpr auto kNewPrimeMeridian = -168.5;

  static std::uint32_t lat_double_to_ufixed(double const lat) noexcept {
    return static_cast<std::uint32_t>(std::round((lat + 90.0) * kCoordinatePrecision));
  }

  static double shift_prime_meridian(double lng) noexcept {
    lng -= kNewPrimeMeridian;
    if (lng >= 0) {
      return lng;
    }
    return lng + 360.0;
  }

  static std::uint32_t lng_double_to_ufixed(double const lng) noexcept {
    return static_cast<std::uint32_t>(std::round(shift_prime_meridian(lng) * kCoordinatePrecision));
  }

  explicit ufixed_latlng(latlng const& c) : lat_{lat_double_to_ufixed(c.lat_)}, lng_{lng_double_to_ufixed(c.lng_)} {}

  std::uint64_t morton_encode() const {
      return libmorton::morton2D_64_encode(lng_, lat_);
  }

  std::uint32_t lat_;
  std::uint32_t lng_;
};

} // namespace geo
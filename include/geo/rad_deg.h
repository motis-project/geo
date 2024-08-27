#pragma once

#include "geo/constants.h"

namespace geo {

constexpr double to_rad(double const deg) { return deg * kPI / 180.0; }
constexpr double to_deg(double const rad) { return rad * 180.0 / kPI; }

inline double operator""_deg(long double const deg) {
  return to_rad(static_cast<double>(deg));
}

}  // namespace geo
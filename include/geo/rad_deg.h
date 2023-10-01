#pragma once

#include "geo/constants.h"

namespace geo {

constexpr double to_rad(double const deg) { return deg * kPI / 180.0; }
constexpr double to_deg(double const rad) { return rad * 180.0 / kPI; }

inline double operator""_rad(long double const deg) { return to_rad(deg); }
inline double operator""_deg(long double const rad) { return to_deg(rad); }

}  // namespace geo
#pragma once

namespace geo {

constexpr auto kPI = 3.14159265358979323846;
constexpr auto kEarthRadiusMeters = 6371000.0;
constexpr auto kEpsilon = 0.000000001;
constexpr auto kApproxDistanceLatDegrees =
    geo::kEarthRadiusMeters * geo::kPI / 180;

}  // namespace geo

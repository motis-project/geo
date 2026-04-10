#pragma once

#include <cstdint>
#include <limits>
#include <variant>

#include "boost/geometry/geometries/point_xy.hpp"

#include "geo/tile.h"

namespace geo {

using fixed_coord_t = int64_t;

using fixed_xy = boost::geometry::model::d2::point_xy<fixed_coord_t>;

constexpr auto invalid_xy = fixed_xy{std::numeric_limits<fixed_coord_t>::max(),
                          std::numeric_limits<fixed_coord_t>::max()};

constexpr fixed_coord_t kFixedCoordMin = 0;
constexpr fixed_coord_t kFixedCoordMax = proj::map_size(kMaxZoomLevel) - 1;
constexpr fixed_coord_t kFixedCoordMagicOffset = kFixedCoordMax / 2ULL;

constexpr auto kFixedDefaultZoomLevel = 20ULL;
static_assert(kFixedDefaultZoomLevel <= kMaxZoomLevel, "invalid default zoom");

}  // namespace geo

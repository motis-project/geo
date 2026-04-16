#pragma once

#include <cstdint>
#include <variant>

#include "mpark/variant.hpp"

#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/geometries/linestring.hpp"
#include "boost/geometry/geometries/multi_linestring.hpp"
#include "boost/geometry/geometries/multi_point.hpp"
#include "boost/geometry/geometries/multi_polygon.hpp"
#include "boost/geometry/geometries/point_xy.hpp"
#include "boost/geometry/geometries/polygon.hpp"

#include "geo/tile.h"

namespace geo {

using fixed_coord_t = std::int64_t;

using fixed_xy = boost::geometry::model::d2::point_xy<fixed_coord_t>;

constexpr auto invalid_xy = fixed_xy{std::numeric_limits<fixed_coord_t>::max(),
                                     std::numeric_limits<fixed_coord_t>::max()};

using fixed_box = boost::geometry::model::box<fixed_xy>;
using fixed_line = boost::geometry::model::linestring<fixed_xy>;
using fixed_simple_polygon = boost::geometry::model::polygon<fixed_xy>;
using fixed_ring = fixed_simple_polygon::ring_type;

constexpr fixed_coord_t kFixedCoordMin = 0;
constexpr fixed_coord_t kFixedCoordMax = proj::map_size(kMaxZoomLevel) - 1;
constexpr fixed_coord_t kFixedCoordMagicOffset = kFixedCoordMax / 2ULL;

constexpr auto kFixedDefaultZoomLevel = 20ULL;
static_assert(kFixedDefaultZoomLevel <= kMaxZoomLevel, "invalid default zoom");

using fixed_delta_t = std::int64_t;

using fixed_null = std::monostate;
using fixed_point = boost::geometry::model::multi_point<fixed_xy>;
using fixed_polyline = boost::geometry::model::multi_linestring<fixed_line>;
using fixed_polygon =
    boost::geometry::model::multi_polygon<fixed_simple_polygon>;

using fixed_geometry =
    mpark::variant<fixed_null, fixed_point, fixed_polyline, fixed_polygon>;

inline fixed_xy latlng_to_fixed(geo::latlng const& pos) {
  auto const px_xy =
      proj::merc_to_pixel(geo::latlng_to_merc(pos), kFixedDefaultZoomLevel);
  return {static_cast<fixed_coord_t>(std::min(
              px_xy.x_, static_cast<geo::pixel_coord_t>(kFixedCoordMax))),
          static_cast<fixed_coord_t>(std::min(
              px_xy.y_, static_cast<geo::pixel_coord_t>(kFixedCoordMax)))};
}

inline geo::latlng fixed_to_latlng(fixed_xy const& pos) {
  return geo::merc_to_latlng(
      {proj::pixel_to_merc_x(pos.x(), kFixedDefaultZoomLevel),
       proj::pixel_to_merc_y(pos.y(), kFixedDefaultZoomLevel)});
}

}  // namespace geo

namespace boost {
namespace geometry {
namespace model {
namespace d2 {

inline bool operator==(point_xy<geo::fixed_coord_t> const& lhs,
                       point_xy<geo::fixed_coord_t> const& rhs) {
  return std::tie(lhs.x(), lhs.y()) == std::tie(rhs.x(), rhs.y());
}

}  // namespace d2
}  // namespace model
}  // namespace geometry
}  // namespace boost

#pragma once

#include <cstdint>
#include <string_view>
#include <vector>

#include "geo/latlng.h"

namespace geo {

// cell of an INSPIRE grid in ETRS89-LAEA (EPSG:3035), all values in meters:
// the cell spans [northing_, northing_ + resolution_) x
// [easting_, easting_ + resolution_)
struct inspire_cell {
  friend bool operator==(inspire_cell const&, inspire_cell const&) = default;

  std::int64_t northing_;
  std::int64_t easting_;
  std::int64_t resolution_;
};

// format: CRS3035RES{resolution}mN{northing}E{easting}
inspire_cell parse_inspire_grid_id(std::string_view);

// the cell with the given resolution (in meters) that contains pos
// exact, unlike lat/lng boxes: a cell is a square in ETRS89-LAEA, but rotated
// against the meridians in lat/lng
inspire_cell inspire_cell_of(latlng const& pos, std::int64_t resolution);

template <typename T>
struct grid_cell {
  inspire_cell id_;
  T data_;
};

template <typename T>
using grid = std::vector<grid_cell<T>>;

grid<std::uint64_t> parse_eurostat_population_grid(std::string_view csv);

}  // namespace geo

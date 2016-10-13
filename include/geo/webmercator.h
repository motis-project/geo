#pragma once

#include <cmath>
#include <iostream>

#include "geo/constants.h"
#include "geo/latlng.h"

//
// These utilities translate between three coordinate systems:
// - latlng: WSG84 coordinates (good ol' GPS coordinates)
// - merc:   EPSG:900913 coordinates (Web Mercator)
// - pixel:  Screen coordinates following Google Schema
//
// Web Mercator:
//
//   -2e7  0   2e7
//   +-----+-----+ 2e7
//   |     |     |
// y +-----------+ 0
// ^ |     |     |
// | +-----+-----+ -2e7
// +--> x
//
// Pixel coordinates with Google tile names at zoom level 1 with 4k tiles
//
// +--> x
// | +-----+-----+ 0
// v | 0/0 | 1/0 |
// y +-----------+ 4k
//   | 0/1 | 1/1 |
//   +-----+-----+ 8k
//   0     4k    8k
//
// more info on:
// http://www.maptiler.org/google-maps-coordinates-tile-bounds-projection/
// https://msdn.microsoft.com/en-us/library/bb259689.aspx
//

namespace geo {

/* +------------------------------------------------------------------------+ */
/* | data types                                                             | */
/* +------------------------------------------------------------------------+ */

using merc_coord_t = double;
using pixel_coord_t = int64_t;

template <typename T>
struct xy {
  xy(T const x, T const y) : x_(x), y_(y) {}
  T x_, y_;
};

using merc_xy = xy<merc_coord_t>;
using pixel_xy = xy<pixel_coord_t>;

template <typename T>
struct bounds {
  bounds() = default;
  bounds(T const minx, T const miny, T const maxx, T const maxy)
      : minx_(minx), miny_(miny), maxx_(maxx), maxy_(maxy) {}

  T minx_, miny_, maxx_, maxy_;
};

using merc_bounds = bounds<merc_coord_t>;
using pixel_bounds = bounds<pixel_coord_t>;

/* +------------------------------------------------------------------------+ */
/* | latlng <-> merc                                                        | */
/* +------------------------------------------------------------------------+ */

constexpr auto kMercEarthRadius = 6378137;
constexpr auto kOriginShift = 2 * kPI * kMercEarthRadius / 2.0;

inline merc_xy latlng_to_merc(latlng const& pos) {
  auto const x = pos.lng_ * kOriginShift / 180.0;
  auto const y =
      (std::log(std::tan((90 + pos.lat_) * kPI / 360.0)) / (kPI / 180.0)) *
      kOriginShift / 180.0;
  return {x, y};
}

/* +------------------------------------------------------------------------+ */
/* | merc <-> pixel / google tile schema                                    | */
/* +------------------------------------------------------------------------+ */

template <int TileSize>
struct webmercator {
  static constexpr auto kInitialResolution =
      2 * kPI * kMercEarthRadius / TileSize;

  // XXX make this constexpr: http://stackoverflow.com/a/34465458
  static double resolution(uint32_t const z) {
    return kInitialResolution / (std::pow(2, z));
  }

  static merc_bounds tile_bounds_merc(uint32_t const x, uint32_t const y,
                                      uint32_t const z) {
    auto const pixel_to_merc = [](uint32_t const p, uint32_t const z) {
      return p * resolution(z) - kOriginShift;
    };

    auto const y_reverse = (std::pow(2, z) - 1) - y;

    auto const minx = pixel_to_merc(x * TileSize, z);
    auto const miny = pixel_to_merc(y_reverse * TileSize, z);
    auto const maxx = pixel_to_merc((x + 1) * TileSize, z);
    auto const maxy = pixel_to_merc((y_reverse + 1) * TileSize, z);
    return {minx, miny, maxx, maxy};
  }

  static pixel_bounds tile_bounds_pixel(uint32_t const x, uint32_t const y,
                                        uint32_t const z) {
    return {x * TileSize, y * TileSize, (x + 1) * TileSize, (y + 1) * TileSize};
  }

  static pixel_coord_t merc_to_pixel_x(merc_coord_t const x, uint32_t const z) {
    return (x + kOriginShift) / resolution(z);
  }

  static pixel_coord_t merc_to_pixel_y(merc_coord_t const y, uint32_t const z) {
    auto const map_size = TileSize << z;  // constexpr
    return map_size - ((y + kOriginShift) / resolution(z));
  }
};

}  // namespace geo

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

struct xy {
  xy(double const x, double const y) : x_(x), y_(y) {}
  double x_, y_;
};

struct bounds {
  bounds() = default;
  bounds(double const minx, double const miny, double const maxx,
         double const maxy)
      : minx_(minx), miny_(miny), maxx_(maxx), maxy_(maxy) {}

  double minx_, miny_, maxx_, maxy_;
};

constexpr auto kMercEarthRadius = 6378137;
constexpr auto kOriginShift = 2 * kPI * kMercEarthRadius / 2.0;

inline xy latlng_to_merc(latlng const& pos) {
  auto const x = pos.lng_ * kOriginShift / 180.0;
  auto const y =
      (std::log(std::tan((90 + pos.lat_) * kPI / 360.0)) / (kPI / 180.0)) *
      kOriginShift / 180.0;
  return {x, y};
}

template <int TileSize>
struct webmercator {
  static constexpr auto kInitialResolution =
      2 * kPI * kMercEarthRadius / TileSize;

  // XXX make this constexpr: http://stackoverflow.com/a/34465458
  static double resolution(uint32_t const z) {
    return kInitialResolution / (std::pow(2, z));
  }

  static bounds tile_bounds_merc(uint32_t const x, uint32_t const y,
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

  static bounds tile_bounds_px(uint32_t const x, uint32_t const y,
                               uint32_t const z) {
    return {x * TileSize, y * TileSize, (x + 1) * TileSize, (y + 1) * TileSize};
  }

  static double merc_to_pixel_x(double x, uint32_t z) {
    return (x + kOriginShift) / resolution(z);
  }

  static double merc_to_pixel_y(double y, uint32_t z) {
    auto const map_size = TileSize << z;  // constexpr
    return map_size - ((y + kOriginShift) / resolution(z));
  }
};

}  // namespace geo

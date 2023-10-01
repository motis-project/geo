#pragma once

#include <cassert>
#include <cmath>
#include <algorithm>
#include <iostream>

#include "geo/constants.h"
#include "geo/latlng.h"
#include "geo/rad_deg.h"

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
  xy() = default;
  explicit xy(T const i) : xy(i, i) {}
  xy(T const x, T const y) : x_(x), y_(y) {}

  T const& x() const { return x_; }
  T const& y() const { return y_; }
  T& x() { return x_; }
  T& y() { return y_; }

  double length() const { return std::sqrt(x_ * x_ + y_ * y_); }

  xy normalize() const {
    auto const len = length();
    auto copy = *this;
    copy.x_ /= len;
    copy.y_ /= len;
    return copy;
  }
  xy normal(bool left = true) const { return left ? xy{-y_, x_} : xy{y_, -x_}; }
  double dot(xy const& o) const { return x_ * o.x_ + y_ * o.y_; }
  double cross(xy const& o) const { return x_ * o.y_ - o.x_ * y_; }

  friend xy operator+(xy const& lhs, xy const& rhs) {
    return {lhs.x() + rhs.x(), lhs.y() + rhs.y()};
  }

  friend xy operator-(xy const& lhs, xy const& rhs) {
    return {lhs.x() - rhs.x(), lhs.y() - rhs.y()};
  }

  friend xy operator*(xy const& v, double s) { return {v.x() * s, v.y() * s}; }
  friend xy operator*(double s, xy const& v) { return v * s; }

  friend void operator*=(xy& v, double s) {
    v.x_ *= s;
    v.y_ *= s;
  }

  friend xy operator/(xy const& v, double s) { return {v.x() / s, v.y() / s}; }

  friend void operator/=(xy& v, double s) {
    v.x_ /= s;
    v.y_ /= s;
  }

  void operator+=(xy const& o) {
    x_ += o.x_;
    y_ += o.y_;
  }

  void operator-=(xy const& o) {
    x_ -= o.x_;
    y_ -= o.y_;
  }

  bool isnan() const { return std::isnan(x_) || std::isnan(y_); }

  friend bool operator==(xy const& a, xy const& b) {
    return std::fabs(a.x_ - b.x_) < kEpsilon &&
           std::fabs(a.y_ - b.y_) < kEpsilon;
  }

  friend bool operator!=(xy const& a, xy const& b) { return !(a == b); }

  friend std::ostream& operator<<(std::ostream& out, xy const& pos) {
    return out << "(" << pos.x_ << ", " << pos.y_ << ")";
  }

  T x_, y_;
};

using merc_xy = xy<merc_coord_t>;
using pixel_xy = xy<pixel_coord_t>;

template <typename T>
struct bounds {
  bounds() = default;
  bounds(T const minx, T const miny, T const maxx, T const maxy)
      : minx_(minx), miny_(miny), maxx_(maxx), maxy_(maxy) {}

  friend bool operator==(bounds<T> const& a, bounds<T> const& b) {
    return std::tie(a.minx_, a.miny_, a.maxx_, a.maxy_) ==
           std::tie(b.minx_, b.miny_, b.maxx_, b.maxy_);
  }

  friend std::ostream& operator<<(std::ostream& out, bounds<T> const& b) {
    return out << "(" << b.minx_ << ", " << b.miny_ << ", " << b.maxx_ << ", "
               << b.maxy_ << ")";
  }

  T minx_, miny_, maxx_, maxy_;
};

using merc_bounds = bounds<merc_coord_t>;
using pixel_bounds = bounds<pixel_coord_t>;

/* +------------------------------------------------------------------------+ */
/* | latlng <-> merc                                                        | */
/* +------------------------------------------------------------------------+ */

constexpr auto kMercEarthRadius = 6378137;
constexpr auto kMercOriginShift = kPI * kMercEarthRadius;
constexpr auto kMercMaxLatitude = 85.0511287798;

inline merc_xy latlng_to_merc(latlng const& pos) {
  auto const lat =
      std::max(std::min(kMercMaxLatitude, pos.lat_), -kMercMaxLatitude);
  auto const sin = std::sin(to_rad(lat));

  return {kMercEarthRadius * to_rad(pos.lng_),
          kMercEarthRadius * std::log((1. + sin) / (1. - sin)) / 2.};
}

inline latlng merc_to_latlng(merc_xy const& xy) {
  constexpr auto d = 180. / kPI;

  return {(2.0 * std::atan(std::exp(xy.y_ / kMercEarthRadius)) - (kPI / 2)) * d,
          xy.x_ * d / kMercEarthRadius};
}

/* +------------------------------------------------------------------------+ */
/* | merc <-> pixel / google tile schema                                    | */
/* +------------------------------------------------------------------------+ */

template <int TileSize, int MaxZoomLevel = 20>
struct webmercator {

  static constexpr auto kTileSize = TileSize;
  static constexpr auto kMaxZoomLevel = MaxZoomLevel;

  static merc_bounds tile_bounds_merc(uint32_t const x, uint32_t const y,
                                      uint32_t const z) {

    auto const pixel_to_merc = [](pixel_coord_t const p, uint32_t const px_z) {
      return resolution(px_z) * p - kMercOriginShift;
    };

    auto const y_reverse = (1ULL << z) - 1 - y;  // 2 ** z - 1 -y

    return {
        pixel_to_merc(static_cast<pixel_coord_t>(x) * TileSize, z),
        pixel_to_merc(static_cast<pixel_coord_t>(y_reverse) * TileSize, z),
        pixel_to_merc(static_cast<pixel_coord_t>(x + 1) * TileSize, z),
        pixel_to_merc(static_cast<pixel_coord_t>(y_reverse + 1) * TileSize, z)};
  }

  static pixel_bounds tile_bounds_pixel(uint32_t const x, uint32_t const y) {
    return {static_cast<pixel_coord_t>(x) * TileSize,
            static_cast<pixel_coord_t>(y) * TileSize,
            static_cast<pixel_coord_t>(x + 1) * TileSize,
            static_cast<pixel_coord_t>(y + 1) * TileSize};
  }

  static pixel_coord_t merc_to_pixel_x(merc_coord_t const x, uint32_t const z) {
    return static_cast<pixel_coord_t>((x + kMercOriginShift) / resolution(z));
  }

  static pixel_coord_t merc_to_pixel_y(merc_coord_t const y, uint32_t const z) {
    return static_cast<pixel_coord_t>(
        std::round(map_size(z) - ((y + kMercOriginShift) / resolution(z))));
  }

  static pixel_xy merc_to_pixel(merc_xy const& merc, uint32_t const z) {
    return {merc_to_pixel_x(merc.x_, z), merc_to_pixel_y(merc.y_, z)};
  }

  constexpr static merc_coord_t pixel_to_merc_x(pixel_coord_t const x,
                                                uint32_t const z) {
    return x * resolution(z) - kMercOriginShift;
  }

  constexpr static merc_coord_t pixel_to_merc_y(pixel_coord_t const y,
                                                uint32_t const z) {
    return (map_size(z) - y) * resolution(z) - kMercOriginShift;
  }

  static merc_xy pixel_to_merc(pixel_xy const& px, uint32_t const z) {
    return {pixel_to_merc_x(px.x_, z), pixel_to_merc_y(px.y_, z)};
  }

  constexpr static double resolution(uint32_t const z) {
    assert(z <= MaxZoomLevel);

    struct look_up_table {
      constexpr look_up_table() : values_() {
        constexpr auto kInitialResolution =
            2 * kPI * kMercEarthRadius / TileSize;
        for (auto i = 0; i <= MaxZoomLevel; ++i) {
          values_[i] = kInitialResolution / (1 << i);
        }
      }

      double values_[MaxZoomLevel + 1];
    };
    constexpr auto lut = look_up_table{};

    return lut.values_[z];
  }

  constexpr static pixel_coord_t map_size(uint32_t const z) {
    assert(z <= MaxZoomLevel);
    return static_cast<pixel_coord_t>(kTileSize) << z;
  }
};

using default_webmercator = webmercator<4096>;

}  // namespace geo

#pragma once

#include "geo/webmercator.h"

namespace geo {

struct tile_range;

struct tile {
  tile() = default;

  tile(uint32_t const x, uint32_t const y, uint32_t const z)
      : x_(x), y_(y), z_(z) {}

  bool operator<(tile const& o) const {
    return std::tie(z_, x_, y_) < std::tie(o.z_, o.x_, o.y_);
  }

  bool operator==(tile const& o) const {
    return std::tie(z_, x_, y_) == std::tie(o.z_, o.x_, o.y_);
  }

  tile_range direct_children() const;

  uint32_t x_, y_, z_;
};

using tile_iterator_bounds = bounds<uint32_t>;
inline tile_iterator_bounds make_no_bounds(uint32_t z) {
  return tile_iterator_bounds{0, 0, 1u << z, 1u << z};
}

struct tile_iterator {
  tile_iterator() : tile_iterator(0) {}

  explicit tile_iterator(uint32_t const z)
      : tile_iterator(z, make_no_bounds(z)) {}

  tile_iterator(uint32_t const z, tile_iterator_bounds bounds)
      : tile_iterator(0, 0, z, bounds) {}

  tile_iterator(uint32_t const x, uint32_t const y, uint32_t const z)
      : tile_iterator(x, y, z, make_no_bounds(z)) {}

  tile_iterator(uint32_t const x, uint32_t const y, uint32_t const z,
                tile_iterator_bounds bounds)
      : tile_(x, y, z), bounds_(bounds) {}

  tile const& operator*() const { return tile_; }
  tile const* operator->() const { return &tile_; }

  tile_iterator& operator++() {
    ++tile_.x_;
    if (tile_.x_ == bounds_.maxx_) {
      tile_.x_ = bounds_.minx_;
      ++tile_.y_;

      if (tile_.y_ == bounds_.maxy_) {
        bounds_.minx_ = bounds_.minx_ << 1;
        bounds_.maxx_ = bounds_.maxx_ << 1;
        bounds_.miny_ = bounds_.miny_ << 1;
        bounds_.maxy_ = bounds_.maxy_ << 1;

        tile_.y_ = bounds_.miny_;
        ++tile_.z_;
      }
    }

    return *this;
  }

  bool operator==(tile_iterator const& o) const {
    return std::tie(tile_.z_, tile_.x_, tile_.y_) ==
           std::tie(o.tile_.z_, o.tile_.x_, o.tile_.y_);
  }
  bool operator!=(tile_iterator const& o) const {
    return std::tie(tile_.z_, tile_.x_, tile_.y_) !=
           std::tie(o.tile_.z_, o.tile_.x_, o.tile_.y_);
  }
  bool operator>(tile_iterator const& o) const {
    return std::tie(tile_.z_, tile_.x_, tile_.y_) >
           std::tie(o.tile_.z_, o.tile_.x_, o.tile_.y_);
  }
  bool operator<(tile_iterator const& o) const {
    return std::tie(tile_.z_, tile_.x_, tile_.y_) <
           std::tie(o.tile_.z_, o.tile_.x_, o.tile_.y_);
  }
  bool operator>=(tile_iterator const& o) const {
    return std::tie(tile_.z_, tile_.x_, tile_.y_) >=
           std::tie(o.tile_.z_, o.tile_.x_, o.tile_.y_);
  }
  bool operator<=(tile_iterator const& o) const {
    return std::tie(tile_.z_, tile_.x_, tile_.y_) <=
           std::tie(o.tile_.z_, o.tile_.x_, o.tile_.y_);
  }

private:
  tile tile_;
  tile_iterator_bounds bounds_;
};

struct tile_range {
  using iterator = tile_iterator;

  tile_range() = default;
  tile_range(iterator begin, iterator end) : begin_(begin), end_(end) {}

  iterator begin() { return begin_; }
  iterator end() { return end_; }

  iterator begin() const { return begin_; }
  iterator end() const { return end_; }

  iterator begin_;
  iterator end_;
};

template <typename Proj = default_webmercator>
tile_range make_tile_range(latlng p_1, latlng p_2, uint32_t z) {
  auto const merc_1 = latlng_to_merc(p_1);
  auto const merc_2 = latlng_to_merc(p_2);
  uint32_t const x_1 = Proj::merc_to_pixel_x(merc_1.x_, z) / Proj::kTileSize;
  uint32_t const x_2 = Proj::merc_to_pixel_x(merc_2.x_, z) / Proj::kTileSize;
  uint32_t const y_1 = Proj::merc_to_pixel_y(merc_1.y_, z) / Proj::kTileSize;
  uint32_t const y_2 = Proj::merc_to_pixel_y(merc_2.y_, z) / Proj::kTileSize;

  tile_iterator_bounds bounds{std::min(x_1, x_2), std::min(y_1, y_2),
                              std::max(x_1, x_2) + 1, std::max(y_1, y_2) + 1};

  return tile_range{
      tile_iterator{std::min(x_1, x_2), std::min(y_1, y_2), z, bounds},
      ++tile_iterator{std::max(x_1, x_2), std::max(y_1, y_2), z, bounds}};
}

template <typename Proj = default_webmercator>
tile_range make_tile_pyramid() {
  return tile_range{tile_iterator{}, tile_iterator{Proj::kMaxZoomLevel + 1}};
}

}  // namespace geo

#pragma once

#include "geo/webmercator.h"

namespace geo {

struct tile_range;
using tile_iterator_bounds = bounds<uint32_t>;

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

  tile parent() const { return {x_ >> 1, y_ >> 1, z_ - 1}; }

  // numbering of the (four) tiles sharing a parent
  inline uint32_t quad_pos() const { return (y_ % 2 << 1) | (x_ % 2); }

  tile_range as_tile_range() const;
  tile_range direct_children() const;
  tile_range range_on_z(uint32_t const z) const;
  tile_iterator_bounds bounds_on_z(uint32_t const z) const;

  friend std::ostream& operator<<(std::ostream& o, tile const& t) {
    return o << "(" << t.x_ << "," << t.y_ << "," << t.z_ << ")";
  }

  uint32_t x_, y_, z_;
};

inline tile_iterator_bounds make_no_bounds(uint32_t z) {
  return tile_iterator_bounds{0, 0, 1u << z, 1u << z};
}

struct tile_iterator {
  tile_iterator() : tile_iterator(0) {}

  explicit tile_iterator(uint32_t const z)
      : tile_iterator(0, 0, z, make_no_bounds(z)) {}

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

        tile_.x_ = bounds_.minx_;
        tile_.y_ = bounds_.miny_;
        ++tile_.z_;
      }
    }

    return *this;
  }

  tile_iterator& operator--() {
    if (tile_.x_ != bounds_.minx_) {
      --tile_.x_;
    } else {
      tile_.x_ = bounds_.maxx_ - 1;

      if (tile_.y_ != bounds_.miny_) {
        --tile_.y_;
      } else {
        bounds_.minx_ = bounds_.minx_ >> 1;
        bounds_.maxx_ = bounds_.maxx_ >> 1;
        bounds_.miny_ = bounds_.miny_ >> 1;
        bounds_.maxy_ = bounds_.maxy_ >> 1;

        tile_.x_ = bounds_.maxx_ - 1;
        tile_.y_ = bounds_.maxy_ - 1;

        assert(tile_.z_ > 0);
        --tile_.z_;
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

  friend tile_range tile_range_on_z(tile_range const&, uint32_t const);

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

// input tile coordinates are inclusive!
tile_range make_tile_range(uint32_t const x_1, uint32_t const y_1,
                           uint32_t const x_2, uint32_t const y_2,
                           uint32_t const z);

// input range must not span zoom levels
tile_range tile_range_on_z(tile_range const&, uint32_t const z);

template <typename Proj = default_webmercator>
tile_range make_tile_range(latlng p_1, latlng p_2, uint32_t z) {
  auto const merc_1 = latlng_to_merc(p_1);
  auto const merc_2 = latlng_to_merc(p_2);
  uint32_t const x_1 = Proj::merc_to_pixel_x(merc_1.x_, z) / Proj::kTileSize;
  uint32_t const x_2 = Proj::merc_to_pixel_x(merc_2.x_, z) / Proj::kTileSize;
  uint32_t const y_1 = Proj::merc_to_pixel_y(merc_1.y_, z) / Proj::kTileSize;
  uint32_t const y_2 = Proj::merc_to_pixel_y(merc_2.y_, z) / Proj::kTileSize;

  return make_tile_range(x_1, y_1, x_2, y_2, z);
}

template <typename Proj = default_webmercator>
tile_range make_tile_pyramid() {
  return tile_range{tile_iterator{}, tile_iterator{Proj::kMaxZoomLevel + 1}};
}

}  // namespace geo

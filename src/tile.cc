#include "geo/tile.h"

namespace geo {

tile_range tile::as_tile_range() const { return range_on_z(z_); }

tile_range tile::direct_children() const { return range_on_z(z_ + 1); }

tile_range tile::range_on_z(uint32_t const z) const {
  if (z_ < z) {
    auto delta_z = z - z_;
    return make_tile_range(x_ << delta_z,  //
                           y_ << delta_z,  //
                           ((x_ + 1) << delta_z) - 1,  //
                           ((y_ + 1) << delta_z) - 1,  //
                           z);
  } else {
    auto delta_z = z_ - z;
    return make_tile_range(x_ >> delta_z, y_ >> delta_z,  //
                           x_ >> delta_z, y_ >> delta_z,  //
                           z);
  }
}

tile_iterator_bounds tile::bounds_on_z(uint32_t const z) const {
  if (z_ < z) {
    auto delta_z = z - z_;
    return tile_iterator_bounds(x_ << delta_z, y_ << delta_z,  //
                                ((x_ + 1) << delta_z), ((y_ + 1) << delta_z));
  } else {
    auto delta_z = z_ - z;
    return tile_iterator_bounds(x_ >> delta_z, y_ >> delta_z,  //
                                (x_ >> delta_z) + 1, (y_ >> delta_z) + 1);
  }
}

tile_range make_tile_range(uint32_t z) {
  auto const bounds = make_no_bounds(z);
  return tile_range{
      tile_iterator{bounds.minx_, bounds.miny_, z, bounds},
      tile_iterator{bounds.minx_ << 1, bounds.miny_ << 1, z + 1, bounds}};
}

tile_range make_tile_range(uint32_t const x_1, uint32_t const y_1,
                           uint32_t const x_2, uint32_t const y_2,
                           uint32_t const z) {
  tile_iterator_bounds bounds{std::min(x_1, x_2), std::min(y_1, y_2),
                              std::max(x_1, x_2) + 1, std::max(y_1, y_2) + 1};
  return tile_range{
      tile_iterator{std::min(x_1, x_2), std::min(y_1, y_2), z, bounds},
      ++tile_iterator{std::max(x_1, x_2), std::max(y_1, y_2), z, bounds}};
}

tile_range tile_range_on_z(tile_range const& range, uint32_t const z) {
  // ATTENTION:  tile_range::bounds [min, max) vs. make_tile_range [min, max]
  auto const tile = range.begin_.tile_;
  auto bounds = range.begin_.bounds_;

  if (tile.z_ < z) {
    auto delta_z = z - tile.z_;
    bounds.minx_ = bounds.minx_ << delta_z;
    bounds.miny_ = bounds.miny_ << delta_z;
    bounds.maxx_ = (bounds.maxx_ << delta_z) - 1;
    bounds.maxy_ = (bounds.maxy_ << delta_z) - 1;
  } else {
    auto delta_z = tile.z_ - z;
    bounds.minx_ = bounds.minx_ >> delta_z;
    bounds.miny_ = bounds.miny_ >> delta_z;
    bounds.maxx_ = (bounds.maxx_ - 1) >> delta_z;
    bounds.maxy_ = (bounds.maxy_ - 1) >> delta_z;
  }

  return make_tile_range(bounds.minx_, bounds.miny_,  //
                         bounds.maxx_, bounds.maxy_,  //
                         z);
}

}  // namespace geo

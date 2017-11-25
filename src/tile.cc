#include "geo/tile.h"

namespace geo {

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

}  // namespace geo

#include "geo/tile.h"

namespace geo {

tile_range tile::direct_children() const {
  auto const x = x_ << 1;
  auto const y = y_ << 1;
  auto const z = z_ + 1;

  tile_iterator_bounds bounds{x, y, x + 2, x + 2};

  return tile_range{tile_iterator{x, y, z, bounds},
                    ++tile_iterator{x + 1, y + 1, z, bounds}};
}

}  // namespace geo

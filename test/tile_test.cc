#include "catch.hpp"

#include <vector>

#include "geo/tile.h"

TEST_CASE("tile::direct_children()") {
  auto const list_tiles = [](auto const& range) {
    std::vector<geo::tile> vec;
    for (auto const& t : range) {
      vec.push_back(t);
    }
    return vec;
  };

  SECTION("root") {
    geo::tile root{0, 0, 0};
    auto const actual = list_tiles(root.direct_children());

    std::vector<geo::tile> expected{{0, 0, 1}, {1, 0, 1}, {0, 1, 1}, {1, 1, 1}};

    CHECK(expected == actual);
  }

  SECTION("darmstadt") {
    geo::tile parent{8585, 5565, 14};
    auto const actual = list_tiles(parent.direct_children());

    std::vector<geo::tile> expected{{17170, 11130, 15},
                                    {17171, 11130, 15},
                                    {17170, 11131, 15},
                                    {17171, 11131, 15}};

    CHECK(expected == actual);
  }
}
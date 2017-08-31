#include "catch.hpp"

#include "geo/webmercator.h"

TEST_CASE("latlng to pixel", "project pixel") {
  using merc256 = geo::webmercator<256>;

  SECTION("projects a center point") {
    auto const merc = geo::latlng_to_merc({0, 0});

    CHECK(merc256::merc_to_pixel_x(merc.x_, 0) == 128);
    CHECK(merc256::merc_to_pixel_y(merc.y_, 0) == 128);
  }

  SECTION("projects the northeast corner of the world") {
    auto const merc = geo::latlng_to_merc({geo::kMercMaxLatitude, 180});
    CHECK(merc256::merc_to_pixel_x(merc.x_, 0) == 256);
    CHECK(merc256::merc_to_pixel_y(merc.y_, 0) == 0);
  }

  SECTION("projects the southwest corner of the world") {
    auto const merc = geo::latlng_to_merc({-geo::kMercMaxLatitude, -180});
    CHECK(merc256::merc_to_pixel_x(merc.x_, 0) == 0);
    CHECK(merc256::merc_to_pixel_y(merc.y_, 0) == 256);
  }
}

TEST_CASE("pixel to latlng", "unproject pixel") {
  using merc256 = geo::webmercator<256>;

  SECTION("reprojects a center point") {
    auto const pos = geo::merc_to_latlng({merc256::pixel_to_merc_x(128, 0),  //
                                          merc256::pixel_to_merc_y(128, 0)});
    CHECK(pos.lat_ == Approx(0.));
    CHECK(pos.lng_ == Approx(0.));
  }

  SECTION("reprojects the northeast corner of the world") {
    auto const pos = geo::merc_to_latlng({merc256::pixel_to_merc_x(256, 0),  //
                                          merc256::pixel_to_merc_y(0, 0)});
    CHECK(pos.lat_ == Approx(geo::kMercMaxLatitude));
    CHECK(pos.lng_ == Approx(180.));
  }

  SECTION("reprojects the southwest corner of the world") {
    auto const pos = geo::merc_to_latlng({merc256::pixel_to_merc_x(0, 0),  //
                                          merc256::pixel_to_merc_y(256, 0)});
    CHECK(pos.lat_ == Approx(-geo::kMercMaxLatitude));
    CHECK(pos.lng_ == Approx(-180.));
  }
}

TEST_CASE("latlng to merc", "project merc") {
  constexpr auto kMercWorldLimit = geo::kMercOriginShift;
  {
    auto const merc = geo::latlng_to_merc({50., 30.});
    CHECK(merc.x_ == Approx(3339584.7238));
    CHECK(merc.y_ == Approx(6446275.84102));
  }
  {
    auto const merc = geo::latlng_to_merc({geo::kMercMaxLatitude, 180.});
    CHECK(merc.x_ == Approx(kMercWorldLimit));
    CHECK(merc.y_ == Approx(kMercWorldLimit));
  }
  {
    auto const merc = geo::latlng_to_merc({-geo::kMercMaxLatitude, -180.});
    CHECK(merc.x_ == Approx(-kMercWorldLimit));
    CHECK(merc.y_ == Approx(-kMercWorldLimit));
  }
}

TEST_CASE("merc to latlng", "unproject merc") {
  constexpr auto kMercWorldLimit = geo::kMercOriginShift;
  {
    auto const pos = geo::merc_to_latlng({3339584.7238, 6446275.84102});
    CHECK(pos.lat_ == Approx(50.));
    CHECK(pos.lng_ == Approx(30.));
  }
  {
    auto const pos = geo::merc_to_latlng({kMercWorldLimit, kMercWorldLimit});
    CHECK(pos.lat_ == Approx(geo::kMercMaxLatitude));
    CHECK(pos.lng_ == Approx(180.));
  }
  {
    auto const pos = geo::merc_to_latlng({-kMercWorldLimit, -kMercWorldLimit});
    CHECK(pos.lat_ == Approx(-geo::kMercMaxLatitude));
    CHECK(pos.lng_ == Approx(-180.));
  }
}


TEST_CASE("webmercator", "reversible") {
  using proj = geo::webmercator<4096>;

  auto const test = [](auto const& input, auto const z) {
    auto const merc_a = proj::pixel_to_merc(input, z);
    auto const latlng = geo::merc_to_latlng(merc_a);
    auto const merc_b = geo::latlng_to_merc(latlng);
    std::cout << latlng.lat_ << " " << latlng.lng_ << std::endl;

    auto const output_a = proj::merc_to_pixel(merc_a, z);
    auto const output_b = proj::merc_to_pixel(merc_b, z);

    CHECK(merc_a.x_ == Approx(merc_b.x_));
    CHECK(merc_a.y_ == Approx(merc_b.y_));
    CHECK(input.x_ == Approx(output_a.x_).epsilon(1));
    CHECK(input.y_ == Approx(output_a.y_).epsilon(1));
    CHECK(input.x_ == Approx(output_b.x_).epsilon(1));
    CHECK(input.y_ == Approx(output_b.y_).epsilon(1));
  };

  test(geo::pixel_xy{0, 0}, 0);
  test(geo::pixel_xy{50, 0}, 0);
  test(geo::pixel_xy{50, 1}, 0);
  test(geo::pixel_xy{50, 2}, 0);
  test(geo::pixel_xy{100, 0}, 0);
  test(geo::pixel_xy{1000, 0}, 0);
}


TEST_CASE("pixel map size") {
  SECTION("tile pyramid") {
    using proj = geo::webmercator<4096>;

    for (auto z = 0u; z <= proj::kMaxZoomLevel; ++z) {
      INFO("zoom level " << z);
      CHECK(proj::map_size(z) == std::pow(2, z) * proj::kTileSize);
    }
  }

  SECTION("32bit limit") {
    using proj = geo::webmercator<4096, 21>;

    CHECK(proj::map_size(19) < std::numeric_limits<uint32_t>::max());
    CHECK(proj::map_size(20) - 1 == std::numeric_limits<uint32_t>::max());
    CHECK(proj::map_size(21) > std::numeric_limits<uint32_t>::max());
  }
}

TEST_CASE("zoom levels") {
  using proj = geo::webmercator<4096>;

  auto const northwest = geo::latlng{geo::kMercMaxLatitude, -180.};
  for (auto z = 0u; z <= proj::kMaxZoomLevel; ++z) {
    INFO("northwest @ zoom level " << z);
    auto const px = proj::merc_to_pixel(geo::latlng_to_merc(northwest), z);
    CHECK(px.x_ == 0);
    CHECK(px.y_ == 0);
  }

  auto const southeast = geo::latlng{-geo::kMercMaxLatitude, 180};
  for (auto z = 0u; z <= proj::kMaxZoomLevel; ++z) {
    INFO("southeast @ zoom level " << z);
    auto const px = proj::merc_to_pixel(geo::latlng_to_merc(southeast), z);
    CHECK(px.x_ == proj::map_size(z));
    CHECK(px.y_ == proj::map_size(z));
  }
}

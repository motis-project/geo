#include "doctest/doctest.h"

#include "geo/box.h"

TEST_CASE("basic_box") {
  geo::polyline line{{49.980557, 9.143697}, {50.002645, 9.072252}};

  auto const sut = geo::box{line};

  CHECK(sut.min_.lat_ == 49.980557);
  CHECK(sut.min_.lng_ == 9.072252);
  CHECK(sut.max_.lat_ == 50.002645);
  CHECK(sut.max_.lng_ == 9.143697);

  CHECK(sut.contains(sut));
  CHECK(sut.contains(geo::make_box({{50.0, 9.1}})));
  CHECK_FALSE(sut.contains(geo::make_box({{49.9, 9.11}, {50.0, 9.12}})));
}

TEST_CASE("box_extend_covers_distance_at_all_latitudes") {
  constexpr auto const kDist = 5000.0;
  constexpr auto const kLngOffset = 0.0675;

  SUBCASE("northern hemisphere") {
    auto const corner = geo::latlng{49.0, 10.0};
    auto const p = geo::latlng{49.0, 10.0 - kLngOffset};
    REQUIRE(geo::distance(corner, p) < kDist);

    auto sut = geo::box{geo::latlng{47.0, 10.0}, geo::latlng{49.0, 11.0}};
    sut.extend(kDist);
    CHECK(sut.contains(p));
  }

  SUBCASE("southern hemisphere") {
    auto const corner = geo::latlng{-49.0, 11.0};
    auto const p = geo::latlng{-49.0, 11.0 + kLngOffset};
    REQUIRE(geo::distance(corner, p) < kDist);

    auto sut = geo::box{geo::latlng{-49.0, 10.0}, geo::latlng{-47.0, 11.0}};
    sut.extend(kDist);
    CHECK(sut.contains(p));
  }
}

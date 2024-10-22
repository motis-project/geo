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

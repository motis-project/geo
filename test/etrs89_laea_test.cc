#include "doctest/doctest.h"

#include "geo/proj_transformers.h"

using namespace geo;

TEST_CASE("from_etrs89_laea") {
  auto transformers = proj_transformers{};

  auto const from_etrs89_laea = [&](auto const easting, auto const northing) {
    return transformers.transform("urn:ogc:def:crs:EPSG::3035", easting, northing);
  };

  auto const darmstadt = from_etrs89_laea(4224037, 2974258);
  CHECK(darmstadt.lng_ == doctest::Approx(8.651222));
  CHECK(darmstadt.lat_ == doctest::Approx(49.872833));

  auto const lisboa = from_etrs89_laea(2665343, 1945115);
  CHECK(lisboa.lng_ == doctest::Approx(-9.13702));
  CHECK(lisboa.lat_ == doctest::Approx(38.707482));

  auto const sulina = from_etrs89_laea(5848671, 2652429);
  CHECK(sulina.lng_ == doctest::Approx(29.654932));
  CHECK(sulina.lat_ == doctest::Approx(45.155896));

  auto const hornstrandir = from_etrs89_laea(2920104, 5141803);
  CHECK(hornstrandir.lng_ == doctest::Approx(-22.443008));
  CHECK(hornstrandir.lat_ == doctest::Approx(66.362433));

  auto const utsjoki = from_etrs89_laea(4974179, 5278480);
  CHECK(utsjoki.lng_ == doctest::Approx(27.025337));
  CHECK(utsjoki.lat_ == doctest::Approx(69.907579));

  auto const valletta = from_etrs89_laea(4732145, 1438675);
  CHECK(valletta.lng_ == doctest::Approx(14.513626));
  CHECK(valletta.lat_ == doctest::Approx(35.899480));
}
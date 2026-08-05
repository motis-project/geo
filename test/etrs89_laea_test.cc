#include "doctest/doctest.h"

#include "geo/proj_transformers.h"

using namespace geo;

TEST_CASE("from_etrs89_laea") {
  auto transformers = proj_transformers{};

  auto const from_etrs89_laea = [&](auto const northing, auto const easting) -> latlng {
    return transformers.transform("urn:ogc:def:crs:EPSG::3035", northing, easting);
  };

  auto const darmstadt = from_etrs89_laea(2974258,4224037);
  CHECK(darmstadt.lat_ == doctest::Approx(49.872833));
  CHECK(darmstadt.lng_ == doctest::Approx(8.651222));


  auto const lisboa = from_etrs89_laea(1945115,2665343);
  CHECK(lisboa.lat_ == doctest::Approx(38.707482));
  CHECK(lisboa.lng_ == doctest::Approx(-9.13702));


  auto const sulina = from_etrs89_laea(2652429,5848671);
  CHECK(sulina.lat_ == doctest::Approx(45.155896));
  CHECK(sulina.lng_ == doctest::Approx(29.654932));


  auto const hornstrandir = from_etrs89_laea(5141803,2920104);
  CHECK(hornstrandir.lat_ == doctest::Approx(66.362433));
  CHECK(hornstrandir.lng_ == doctest::Approx(-22.443008));

  auto const utsjoki = from_etrs89_laea( 5278480,4974179);
  CHECK(utsjoki.lat_ == doctest::Approx(69.907579));
  CHECK(utsjoki.lng_ == doctest::Approx(27.025337));


  auto const valletta = from_etrs89_laea( 1438675,4732145);
  CHECK(valletta.lat_ == doctest::Approx(35.899480));
  CHECK(valletta.lng_ == doctest::Approx(14.513626));
}
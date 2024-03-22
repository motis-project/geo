#include "doctest/doctest.h"

#include "geo/latlng.h"

TEST_CASE("destination_point") {
  auto constexpr source1 = geo::latlng{40.0, -20.0};
  auto constexpr distance1 = 111800.0;
  auto constexpr bearing1 = 0.0;
  auto constexpr destination1_exp = geo::latlng{41.00555556, -20.0};
  auto const destination1_act = geo::destination_point(source1, distance1, bearing1);
  CHECK(destination1_act.lat_ == doctest::Approx(destination1_exp.lat_));
  CHECK(destination1_act.lng_ == doctest::Approx(destination1_exp.lng_));

  auto constexpr source2 = geo::latlng{-23.0, 42.0};
  auto constexpr distance2 = 2342000.0;
  auto constexpr bearing2 = 90.0;
  auto constexpr destination2_exp = geo::latlng{-21.38472222, 64.70277777};
  auto const destination2_act = geo::destination_point(source2, distance2, bearing2);
  CHECK(destination2_act.lat_ == doctest::Approx(destination2_exp.lat_));
  CHECK(destination2_act.lng_ == doctest::Approx(destination2_exp.lng_));

  auto constexpr source3 = geo::latlng{89.0, 3.0};
  auto constexpr distance3 = 11111000.0;
  auto constexpr bearing3 = 77.0;
  auto constexpr destination3_exp = geo::latlng{-9.69722222, 106.16833333};
  auto const destination3_act = geo::destination_point(source3, distance3, bearing3);
  CHECK(destination3_act.lat_ == doctest::Approx(destination3_exp.lat_));
  CHECK(destination3_act.lng_ == doctest::Approx(destination3_exp.lng_));
}

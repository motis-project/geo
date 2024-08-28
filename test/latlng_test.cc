#include "doctest/doctest.h"

#include <array>
#include <vector>

#include "geo/latlng.h"

TEST_CASE("destination_point") {
  auto constexpr source1 = geo::latlng{40.0, -20.0};
  auto constexpr distance1 = 111800.0;
  auto constexpr bearing1 = 0.0;
  auto constexpr destination1_exp = geo::latlng{41.00555556, -20.0};
  auto const destination1_act =
      geo::destination_point(source1, distance1, bearing1);
  CHECK(destination1_act.lat_ == doctest::Approx(destination1_exp.lat_));
  CHECK(destination1_act.lng_ == doctest::Approx(destination1_exp.lng_));

  auto constexpr source2 = geo::latlng{-23.0, 42.0};
  auto constexpr distance2 = 2342000.0;
  auto constexpr bearing2 = 90.0;
  auto constexpr destination2_exp = geo::latlng{-21.38472222, 64.70277777};
  auto const destination2_act =
      geo::destination_point(source2, distance2, bearing2);
  CHECK(destination2_act.lat_ == doctest::Approx(destination2_exp.lat_));
  CHECK(destination2_act.lng_ == doctest::Approx(destination2_exp.lng_));

  auto constexpr source3 = geo::latlng{89.0, 3.0};
  auto constexpr distance3 = 11111000.0;
  auto constexpr bearing3 = 77.0;
  auto constexpr destination3_exp = geo::latlng{-9.69722222, 106.16833333};
  auto const destination3_act =
      geo::destination_point(source3, distance3, bearing3);
  CHECK(destination3_act.lat_ == doctest::Approx(destination3_exp.lat_));
  CHECK(destination3_act.lng_ == doctest::Approx(destination3_exp.lng_));
}

TEST_CASE("latlngClosestOnSegment_pointsNotOnSegment_getStartOrEnd") {
  auto const tests =
      std::vector<std::tuple<geo::latlng, geo::latlng, geo::latlng, bool>>{
          // simple coordinates near start
          {{0.0, 0.0}, {1.0, 0.0}, {0.0, 1.0}, true},
          {{0.0, 0.0}, {1.0, 0.0}, {0.0, -1.0}, true},
          {{0.0, 0.0}, {1.0, 0.0}, {-1.0, 0.0}, true},
          {{0.0, 0.0}, {1.0, 0.0}, {-0.5, 0.5}, true},
          // simple coordinates near end
          {{0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0}, false},
          {{0.0, 0.0}, {1.0, 0.0}, {1.0, -1.0}, false},
          {{0.0, 0.0}, {1.0, 0.0}, {2.0, 0.0}, false},
          {{0.0, 0.0}, {1.0, 0.0}, {1.5, 0.5}, false},
          // larger distances
          {{50.0, 0.0}, {90.0, 180.0}, {39.0, 10.0}, true},
          {{0.0, 0.0}, {0.0, 90.0}, {0.0, 120.0}, false},
          // random coordinates
          {{24.427, -163.318}, {46.629, -85.595}, {83.911, -7.324}, false},
          {{-54.556, 66.671}, {-68.925, -70.823}, {36.411, 97.435}, true},
          {{-12.087, 53.036}, {-63.395, -104.788}, {-15.509, -137.375}, false},
          {{48.384, 3.970}, {-86.712, -147.266}, {66.293, 3.294}, true},
          {{-26.293, 83.294}, {63.181, -44.492}, {10.135, -159.263}, false},
          // (random) coordinates in Europe
          {{35.700, 17.598}, {57.153, 28.220}, {65.793, 36.753}, false},
          {{48.507, 17.041}, {37.068, 48.912}, {53.585, 1.913}, true},
      };
  for (auto const& [from, to, x, is_start] : tests) {
    auto const closest = geo::closest_on_segment(x, from, to);

    auto const& expected = is_start ? from : to;
    CHECK(expected == closest);
  }
}

TEST_CASE(
    "latlngClosestOnSegment_pointsCloseToSegment_getReasonableCandidate") {
  auto const tests = std::vector<std::array<geo::latlng, 3>>{
      // simple coordinates
      {{{0.0, 0.0}, {1.0, 0.0}, {0.1, 0.0}}},
      {{{0.0, 0.0}, {1.0, 0.0}, {0.9, 0.0}}},
      {{{0.0, 0.0}, {1.0, 0.0}, {0.5, 0.0}}},
      {{{0.0, 0.0}, {1.0, 0.0}, {0.5, 0.2}}},
      // close to segment
      {{{-59.0, 54.0}, {-67.0, 26.0}, {-62.7, 42.0}}},
      {{{1.0, 1.0}, {1.0020, 1.0005}, {1.0010, 1.0011}}},
      // (random) coordinates in Europe
      {{{37.3908, 8.3000}, {67.8311, 39.7556}, {41.4347, 27.7353}}},
      {{{37.2922, 25.0194}, {57.9814, 5.5728}, {50.6969, 5.3486}}},
      {{{40.303, 45.234}, {46.657, 15.126}, {34.561, 41.347}}},
      {{{36.377, 25.299}, {48.428, 39.082}, {43.046, 24.722}}},
      {{{49.885, 22.146}, {70.732, 29.241}, {61.708, 8.514}}},
      {{{37.864, 38.041}, {50.021, 7.588}, {41.093, 8.343}}},
      {{{49.660, -5.444}, {66.286, 48.260}, {41.630, 9.008}}},
  };
  for (auto const& [from, to, x] : tests) {
    auto const closest = geo::closest_on_segment(x, from, to);

    CHECK(!(closest == from));
    CHECK(!(closest == to));
    CHECK(geo::distance(x, closest) < geo::distance(x, from));
    CHECK(geo::distance(x, closest) < geo::distance(x, to));
  }
}
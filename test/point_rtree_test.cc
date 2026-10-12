#include "doctest/doctest.h"

#include "geo/latlng.h"

#include "geo/point_rtree.h"

TEST_CASE("point rtree") {
  std::vector<geo::latlng> points;

  points.push_back(geo::latlng{49.8726016, 8.6310396});  // Hauptbahnhof
  points.push_back(geo::latlng{49.8728246, 8.6512529});  // Luisenplatz
  points.push_back(geo::latlng{49.8780513, 8.6547033});  // Algo Offices

  auto const rtree = make_point_rtree(points, [](auto const& e) { return e; });

  auto const mensa = geo::latlng{49.8756276, 8.6577833};

  SUBCASE("finds algo") {
    auto const r = rtree.in_radius(mensa, 450);
    REQUIRE(r.size() == 1);
    CHECK(r[0] == 2);
  }

  SUBCASE("finds lui") {
    auto const r = rtree.in_radius(mensa, 450, 1000);
    REQUIRE(r.size() == 1);
    CHECK(r[0] == 1);
  }

  SUBCASE("finds all ordered") {
    auto const r = rtree.in_radius_with_distance(mensa, 10000);
    REQUIRE(r.size() == 3);
    CHECK(r[0].second == 2);
    CHECK(r[1].second == 1);
    CHECK(r[2].second == 0);

    CHECK(r[0].first <= r[1].first);
    CHECK(r[1].first <= r[2].first);
    CHECK(r[0].first <= r[2].first);
  }
}

TEST_CASE("point rtree in_radius north and south at the radius") {
  constexpr auto kRadius = 10'000.0;
  // a degree of latitude is shortest at the equator (WGS84 meridian)
  for (auto const lat : {0.0, 20.0, 35.0, 60.0, -35.0}) {
    auto const center = geo::latlng{lat, 8.0};
    for (auto const dir : {1.0, -1.0}) {
      // the point straight north/south of center that is just inside kRadius
      auto inside = 0.0, outside = 0.2;
      for (auto i = 0; i != 60; ++i) {
        auto const mid = (inside + outside) / 2.0;
        if (geo::distance(center, {lat + dir * mid, 8.0}) < kRadius) {
          inside = mid;
        } else {
          outside = mid;
        }
      }
      auto const p = geo::latlng{lat + dir * inside, 8.0};
      REQUIRE(geo::distance(center, p) < kRadius);
      REQUIRE(geo::distance(center, p) > kRadius - 0.01);

      auto const rtree = geo::make_point_rtree(std::vector{p});
      CHECK(rtree.in_radius(center, kRadius).size() == 1U);
    }
  }
}

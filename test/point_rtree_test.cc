#include "catch2/catch.hpp"

#include "geo/point_rtree.h"

TEST_CASE("point rtree") {
  std::vector<geo::latlng> points;

  points.emplace_back(49.8726016, 8.6310396);  // Hauptbahnhof
  points.emplace_back(49.8728246, 8.6512529);  // Luisenplatz
  points.emplace_back(49.8780513, 8.6547033);  // Algo Offices

  auto const rtree = make_point_rtree(points, [](auto const& e) { return e; });

  auto const mensa = geo::latlng{49.8756276, 8.6577833};

  SECTION("finds algo") {
    auto const r = rtree.in_radius(mensa, 450);
    REQUIRE(r.size() == 1);
    CHECK(r[0] == 2);
  }

  SECTION("finds lui") {
    auto const r = rtree.in_radius(mensa, 450, 1000);
    REQUIRE(r.size() == 1);
    CHECK(r[0] == 1);
  }

  SECTION("finds all ordered") {
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
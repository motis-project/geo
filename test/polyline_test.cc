#include "doctest/doctest.h"

#include <ranges>

#include "geo/constants.h"
#include "geo/latlng.h"

#include "geo/polyline.h"

using namespace geo;

double abs(double const x) { return x < 0 ? -x : x; }

TEST_CASE("polylineDistanceToPolyline_pointBeforeLine_getStartPoint") {
  auto const graph = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}};
  auto const test_point = latlng{-1.0f, 0.0f};

  auto const closest = distance_to_polyline(test_point, graph);

  auto const start_coordinate = *graph.begin();
  CHECK(closest.distance_to_polyline_ ==
        distance(test_point, start_coordinate));
  CHECK(closest.best_ == start_coordinate);
}

TEST_CASE("polylineDistanceToPolyline_pointAfterLine_getEndPoint") {
  auto const graph = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}};
  auto const test_point = latlng{2.0f, 0.0f};

  auto const closest = distance_to_polyline(test_point, graph);

  auto const end_coordinate = *graph.rbegin();
  CHECK(closest.segment_idx_ == 0);
  CHECK(closest.distance_to_polyline_ == distance(test_point, end_coordinate));
  CHECK(closest.best_ == end_coordinate);
}

TEST_CASE("polylineDistanceToPolyline_pointOnLine_getPointOnLine") {
  auto const graph =
      polyline{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
  auto const test_point = latlng{0.5f, 0.0f};

  auto const closest = distance_to_polyline(test_point, graph);

  CHECK(closest.distance_to_polyline_ < kEpsilon);
  CHECK(closest.segment_idx_ == 0);
  CHECK(distance(closest.best_, test_point) < kEpsilon);
}

TEST_CASE("polylineDistanceToPolyline_pointNotOnLine_getClosestPoint") {
  auto const graph =
      polyline{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
  auto const test_point = latlng{0.75f, 0.5f};

  auto const closest = distance_to_polyline(test_point, graph);

  auto const best = latlng{1.0f, 0.5f};
  CHECK(closest.segment_idx_ == 1);
  CHECK(abs(distance(test_point, best) - distance(test_point, closest.best_)) <
        kEpsilon);
  CHECK(abs(closest.distance_to_polyline_ - distance(test_point, best)) <
        kEpsilon);
  CHECK(distance(closest.best_, best) < kEpsilon);
}

#include "doctest/doctest.h"

#include <ranges>

#include "geo/constants.h"
#include "geo/latlng.h"

#include "geo/polyline.h"

using namespace geo;

double abs(double const x) {
    return x < 0 ? -x : x;
}

TEST_CASE("polylineDistanceToPolyline_pointBeforeLine_getStartPoint") {
  auto const line = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}};
  auto const test_point = latlng{-1.0f, 0.0f};

  auto const projection = distance_to_polyline(test_point, line);

  auto const start_coordinate = *line.begin();
  CHECK(projection.distance_to_polyline_ == distance(test_point, start_coordinate));
  CHECK(projection.best_ == start_coordinate);
}

TEST_CASE("polylineDistanceToPolyline_pointAfterLine_getEndPoint") {
  auto const line = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}};
  auto const test_point = latlng{2.0f, 0.0f};

  auto const projection = distance_to_polyline(test_point, line);

  auto const end_coordinate = *line.rbegin();
  CHECK(projection.segment_idx_ == 0);
  CHECK(projection.distance_to_polyline_ == distance(test_point, end_coordinate));
  CHECK(projection.best_ == end_coordinate);
}

TEST_CASE("polylineDistanceToPolyline_pointOnLine_getPointOnLine") {
  auto const line = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
  auto const test_point = latlng{0.5f, 0.0f};

  auto const projection = distance_to_polyline(test_point, line);

  CHECK(projection.distance_to_polyline_ < kEpsilon);
  CHECK(projection.segment_idx_ == 0);
  CHECK(distance(projection.best_, test_point) < kEpsilon);
}

TEST_CASE("polylineDistanceToPolyline_pointNotOnLine_getClosestPoint") {
  auto const line = polyline{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}};
  auto const test_point = latlng{0.75f, 0.5f};

  auto const projection = distance_to_polyline(test_point, line);

  auto const best = latlng{1.0f, 0.5f};
  CHECK(projection.segment_idx_ == 1);
  CHECK(abs(distance(test_point, best) - distance(test_point, projection.best_)) < kEpsilon);
  CHECK(abs(projection.distance_to_polyline_ - distance(test_point, best)) < kEpsilon);
  CHECK(distance(projection.best_, best) < kEpsilon);
}

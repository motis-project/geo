#include "doctest/doctest.h"

#include <cmath>

#include "geo/constants.h"
#include "geo/latlng.h"

#include "geo/polyline.h"

using namespace geo;

TEST_CASE("polylineDistanceToPolyline_pointBeforeLine_getStartPoint") {
  auto const graph = polyline{{0.0F, 0.0F}, {1.0F, 0.0F}};
  auto const test_point = latlng{-1.0F, 0.0F};

  auto const closest = distance_to_polyline(test_point, graph);

  auto const start_coordinate = graph.front();
  CHECK(closest.distance_to_polyline_ ==
        distance(test_point, start_coordinate));
  CHECK(closest.best_ == start_coordinate);
}

TEST_CASE("polylineDistanceToPolyline_pointAfterLine_getEndPoint") {
  auto const graph = polyline{{0.0F, 0.0F}, {1.0F, 0.0F}};
  auto const test_point = latlng{2.0F, 0.0F};

  auto const closest = distance_to_polyline(test_point, graph);

  auto const end_coordinate = graph.back();
  CHECK(closest.segment_idx_ == 0);
  CHECK(closest.distance_to_polyline_ == distance(test_point, end_coordinate));
  CHECK(closest.best_ == end_coordinate);
}

TEST_CASE("polylineDistanceToPolyline_pointOnLine_getPointOnLine") {
  auto const graph =
      polyline{{0.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 1.0F}};
  auto const test_point = latlng{0.5F, 0.0F};

  auto const closest = distance_to_polyline(test_point, graph);

  CHECK(closest.distance_to_polyline_ < kEpsilon);
  CHECK(closest.segment_idx_ == 0);
  CHECK(distance(closest.best_, test_point) < kEpsilon);
}

TEST_CASE("polylineDistanceToPolyline_pointNotOnLine_getClosestPoint") {
  auto const graph =
      polyline{{0.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 1.0F}};
  auto const test_point = latlng{0.75F, 0.5F};

  auto const closest = distance_to_polyline(test_point, graph);

  auto const best = latlng{1.0F, 0.5F};
  CHECK(closest.segment_idx_ == 1);
  CHECK(std::abs(distance(test_point, best) -
                 distance(test_point, closest.best_)) < kEpsilon);
  CHECK(std::abs(closest.distance_to_polyline_ - distance(test_point, best)) <
        kEpsilon);
  CHECK(distance(closest.best_, best) < kEpsilon);
}

TEST_CASE(
    "polylineSplitPolyline_multipleCoordinates_getCorrectSegmentsForEach") {
  auto const graph =
      polyline{{0.0F, 0.0F}, {1.0F, 0.0F}, {1.0F, 1.0F}, {0.0F, 1.0F},
               {0.4F, 1.4F}, {1.4F, 1.4F}, {1.4F, 0.4F}, {0.4F, 0.4F}};
  auto const test_entries =
      std::vector<std::tuple<geo::latlng, geo::latlng, std::size_t>>{
          {{0.99F, 0.95F}, {1.0F, 0.95F}, 1U},
          {{0.9F, 1.01F}, {0.9F, 1.0F}, 2U},
          {{0.6F, 0.9F}, {0.6F, 1.0F}, 2U},
          {{0.41F, 1.39F}, {0.41F, 1.40F}, 4U},
          {{1.0F, 1.5F}, {1.0F, 1.4F}, 4U},
          {{1.39F, 1.4F}, {1.39F, 1.4F}, 4U},
          {{1.41F, 0.6F}, {1.4F, 0.6F}, 5U},
      };

  for (auto const& test_entry : test_entries) {
    auto const& [test_point, expected_point, expected_segment] = test_entry;

    auto const closest = distance_to_polyline(test_point, graph);

    CHECK(closest.segment_idx_ == expected_segment);
    CHECK(distance(closest.best_, expected_point) < 2 * kEpsilon);
  }
}

#include "catch.hpp"

#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "geo/latlng.h"
#include "geo/xyz.h"

namespace sc = std::chrono;

#define GEO_START_TIMING(_X) \
  auto _X##_start = sc::steady_clock::now(), _X##_stop = _X##_start
#define GEO_STOP_TIMING(_X) _X##_stop = sc::steady_clock::now()
#define GEO_TIMING_MS(_X) \
  (sc::duration_cast<sc::milliseconds>(_X##_stop - _X##_start).count())

TEST_CASE("xyz haversine_distance") {
  constexpr auto kSize = 100;  // increase this number for perf eval
  std::vector<geo::latlng> latlng_pos;
  latlng_pos.reserve(kSize);

  {
    std::mt19937 gen(0);
    std::uniform_real_distribution<> lat_dist{0., 180.};
    std::uniform_real_distribution<> lng_dist{0., 360.};

    for (auto i = 0; i < kSize; ++i) {
      latlng_pos.emplace_back(lat_dist(gen) - 90.0, lng_dist(gen) - 180.0);
    }
  }

  std::vector<geo::xyz> xyz_pos;
  xyz_pos.reserve(kSize);
  GEO_START_TIMING(xyz_conv);
  for (auto i = 0; i < kSize; ++i) {
    xyz_pos.emplace_back(latlng_pos[i]);
  }
  GEO_STOP_TIMING(xyz_conv);
  std::cout << "latlng2xyz conv: " << GEO_TIMING_MS(xyz_conv) << " ms\n";

  SECTION("haversine_distance") {
    GEO_START_TIMING(latlng_dists);
    std::vector<std::vector<double>> latlng_dists;
    for (auto const& pos_a : latlng_pos) {
      std::vector<double> row;
      for (auto const& pos_b : latlng_pos) {
        row.emplace_back(distance(pos_a, pos_b));
      }
      latlng_dists.emplace_back(std::move(row));
    }
    GEO_STOP_TIMING(latlng_dists);
    std::cout << "latlng matrix: " << GEO_TIMING_MS(latlng_dists) << " ms\n";

    GEO_START_TIMING(xyz_dists);
    std::vector<std::vector<double>> xyz_dists;
    for (auto const& pos_a : xyz_pos) {
      std::vector<double> row;
      for (auto const& pos_b : xyz_pos) {
        row.emplace_back(haversine_distance(pos_a, pos_b));
      }
      xyz_dists.emplace_back(std::move(row));
    }
    GEO_STOP_TIMING(xyz_dists);
    std::cout << "xyz matrix: " << GEO_TIMING_MS(xyz_dists) << " ms\n";

    REQUIRE(latlng_dists.size() == xyz_dists.size());
    for (auto i = 0; i < kSize; ++i) {
      REQUIRE(latlng_dists[i].size() == xyz_dists[i].size());
      for (auto j = 0; j < kSize; ++j) {
        CHECK(latlng_dists[i][j] == Approx(xyz_dists[i][j]));
      }
    }
  }

  SECTION("min_haversine_distance") {
    std::vector<std::vector<geo::latlng>> latlng_pos_skip_self;
    {
      for (auto i = 0; i < kSize; ++i) {
        std::vector<geo::latlng> row;
        for (auto j = 0; j < kSize; ++j) {
          if (i == j) {
            continue;
          }
          row.emplace_back(latlng_pos[j]);
        }
        latlng_pos_skip_self.emplace_back(std::move(row));
      }
    }

    GEO_START_TIMING(latlng_min);
    std::vector<double> latlng_min;
    for (auto i = 0u; i < kSize; ++i) {
      auto min_d = std::numeric_limits<double>::infinity();
      for (auto const& pos_b : latlng_pos_skip_self[i]) {
        auto const d = distance(latlng_pos[i], pos_b);
        if (d < min_d) {
          min_d = d;
        }
      }
      latlng_min.emplace_back(min_d);
    }
    GEO_STOP_TIMING(latlng_min);
    std::cout << "latlng min: " << GEO_TIMING_MS(latlng_min) << " ms\n";

    std::vector<std::vector<geo::xyz>> xyz_pos_skip_self;
    {
      for (auto i = 0; i < kSize; ++i) {
        std::vector<geo::xyz> row;
        for (auto j = 0; j < kSize; ++j) {
          if (i == j) {
            continue;
          }
          row.emplace_back(xyz_pos[j]);
        }
        xyz_pos_skip_self.emplace_back(std::move(row));
      }
    }

    GEO_START_TIMING(xyz_min);
    std::vector<double> xyz_min;
    for (auto i = 0u; i < kSize; ++i) {
      xyz_min.emplace_back(
          min_haversine_distance(xyz_pos[i], xyz_pos_skip_self[i]));
    }
    GEO_STOP_TIMING(xyz_min);
    std::cout << "xyz min: " << GEO_TIMING_MS(xyz_min) << " ms\n";

    REQUIRE(latlng_min.size() == xyz_min.size());
    for (auto i = 0; i < kSize; ++i) {
      CHECK(latlng_min[i] == Approx(xyz_min[i]));
    }
  }
}

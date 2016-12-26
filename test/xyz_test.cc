#include "catch.hpp"

#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include "geo/latlng.h"
#include "geo/xyz.h"

#define GEO_START_TIMING(_X) \
  auto _X##_start = std::chrono::steady_clock::now(), _X##_stop = _X##_start
#define GEO_STOP_TIMING(_X) _X##_stop = std::chrono::steady_clock::now()
#define GEO_TIMING_MS(_X)                                            \
  (std::chrono::duration_cast<std::chrono::milliseconds>(_X##_stop - \
                                                         _X##_start) \
       .count())

TEST_CASE("xyz") {
  constexpr auto kSize = 250; // increase this number for perf eval
  std::vector<geo::latlng> latlng_pos;
  latlng_pos.reserve(kSize);

  {
    std::mt19937 gen(0);
    std::uniform_real_distribution<> lat_dist{90., std::nexttoward(-90., -91.)};
    std::uniform_real_distribution<> lng_dist{180., -180.};

    for (auto i = 0; i < kSize; ++i) {
      latlng_pos.emplace_back(lat_dist(gen), lng_dist(gen));
    }
  }

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
  std::cout << "latlng matrix took: " << GEO_TIMING_MS(latlng_dists) << " ms\n";

  GEO_START_TIMING(xyz_conv);
  std::vector<geo::xyz> xyz_pos;
  xyz_pos.reserve(kSize);
  for (auto i = 0; i < kSize; ++i) {
    xyz_pos.emplace_back(latlng_pos[i]);
  }
  GEO_STOP_TIMING(xyz_conv);
  std::cout << "latlng2xyz conversion: " << GEO_TIMING_MS(xyz_conv) << " ms\n";

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
  std::cout << "xyz matrix took: " << GEO_TIMING_MS(xyz_dists) << " ms\n";

  for (auto i = 0; i < kSize; ++i) {
    for (auto j = 0; j < kSize; ++j) {
      CHECK(latlng_dists[i][j] == Approx(xyz_dists[i][j]));
    }
  }
}

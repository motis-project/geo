#include "geo/grid.h"

#include "doctest/doctest.h"

using namespace geo;

constexpr auto kGridId = "CRS3035RES1000mN2683000E4285000";

constexpr auto kGridCSV =
    R"(GRD_ID,T,M,F,Y_LT15,Y_1564,Y_GE65,EMP,NAT,EU_OTH,OTH,SAME,CHG_IN,CHG_OUT,LAND_SURFACE,POPULATED,CNTR_ID
CRS3035RES1000mN2683000E4285000,0,0,0,0,0,0,0,0,0,0,0,0,0,0.9471130000000001,0,AT-CH-LI
CRS3035RES1000mN2684000E4285000,25,14,12,6,18,3,2,22,0,1,23,0,0,0.903667,1,AT-CH-LI
CRS3035RES1000mN2682000E4286000,0,0,0,0,0,0,0,0,0,0,0,0,0,0.999,0,AT-LI
CRS3035RES1000mN2683000E4286000,116,63,53,19,79,18,54,96,9,11,106,7,0,0.9899,1,AT-LI
CRS3035RES1000mN2684000E4286000,185,96,89,51,113,21,89,152,22,11,161,16,4,0.961884,1,AT-CH)";

TEST_CASE("parse_inspire_grid_id") {
  auto const b = parse_inspire_grid_id(kGridId);
  CHECK_EQ(b.min_.lat_, doctest::Approx(47.2595447));
  CHECK_EQ(b.min_.lng_, doctest::Approx(9.5247167));
  CHECK_EQ(b.max_.lat_, doctest::Approx(47.2686031));
  CHECK_EQ(b.max_.lng_, doctest::Approx(9.537839));
}

TEST_CASE("population_grid") {
  auto const g = parse_eurostat_population_grid(kGridCSV);

  REQUIRE_EQ(g.size(), 5U);

  CHECK_EQ(g[0].b_.min_.lat_, doctest::Approx(47.2595447));
  CHECK_EQ(g[0].b_.min_.lng_, doctest::Approx(9.5247167));
  CHECK_EQ(g[0].b_.max_.lat_, doctest::Approx(47.2686031));
  CHECK_EQ(g[0].b_.max_.lng_, doctest::Approx(9.537839));
  CHECK_EQ(g[0].data_, 0U);

  CHECK_EQ(g[1].b_.min_.lat_, doctest::Approx(47.2685461));
  CHECK_EQ(g[1].b_.min_.lng_, doctest::Approx(9.5246347));
  CHECK_EQ(g[1].b_.max_.lat_, doctest::Approx(47.2776044));
  CHECK_EQ(g[1].b_.max_.lng_, doctest::Approx(9.5377592));
  CHECK_EQ(g[1].data_, 25U);

  CHECK_EQ(g[2].b_.min_.lat_, doctest::Approx(47.2506002));
  CHECK_EQ(g[2].b_.min_.lng_, doctest::Approx(9.5379984));
  CHECK_EQ(g[2].b_.max_.lat_, doctest::Approx(47.259657));
  CHECK_EQ(g[2].b_.max_.lng_, doctest::Approx(9.5511207));
  CHECK_EQ(g[2].data_, 0U);

  CHECK_EQ(g[3].b_.min_.lat_, doctest::Approx(47.2596016));
  CHECK_EQ(g[3].b_.min_.lng_, doctest::Approx(9.5379187));
  CHECK_EQ(g[3].b_.max_.lat_, doctest::Approx(47.2686584));
  CHECK_EQ(g[3].b_.max_.lng_, doctest::Approx(9.5510433));
  CHECK_EQ(g[3].data_, 116U);

  CHECK_EQ(g[4].b_.min_.lat_, doctest::Approx(47.2686031));
  CHECK_EQ(g[4].b_.min_.lng_, doctest::Approx(9.537839));
  CHECK_EQ(g[4].b_.max_.lat_, doctest::Approx(47.2776598));
  CHECK_EQ(g[4].b_.max_.lng_, doctest::Approx(9.5509658));
  CHECK_EQ(g[4].data_, 185U);
}

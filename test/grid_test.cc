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
  CHECK(b.min_.lat_ == doctest::Approx(47.2595447));
  CHECK(b.min_.lng_ == doctest::Approx(9.5247167));
  CHECK(b.max_.lat_ == doctest::Approx(47.2686031));
  CHECK(b.max_.lng_ == doctest::Approx(9.537839));
}

TEST_CASE("population_grid") {}
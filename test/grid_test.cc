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

// cell in Lisbon, far from 10°E where cells are strongly rotated in lat/lng
constexpr auto kLisbonCell = inspire_cell{
    .northing_ = 1945000, .easting_ = 2665000, .resolution_ = 1000};
// inside kLisbonCell, but outside the lat/lng box spanned by its south-west
// and north-east corners
constexpr auto kLisbonNearSE = latlng{38.708022895, -9.129515763};
constexpr auto kLisbonNearNW = latlng{38.714379763, -9.142995004};
constexpr auto kLisbonCenter = latlng{38.711201591, -9.136255027};
// 1 m south and 1 m west of kLisbonCell's south-west corner
constexpr auto kLisbonOutsideSW = latlng{38.705687937, -9.140562270};

TEST_CASE("parse_inspire_grid_id") {
  auto const c = parse_inspire_grid_id(kGridId);
  CHECK_EQ(c.northing_, 2683000);
  CHECK_EQ(c.easting_, 4285000);
  CHECK_EQ(c.resolution_, 1000);

  CHECK_THROWS(parse_inspire_grid_id("CRS4326RES1000mN2683000E4285000"));
  CHECK_THROWS(parse_inspire_grid_id("DE_unallocated"));
}

TEST_CASE("inspire_cell_of") {
  CHECK_EQ(inspire_cell_of(kLisbonCenter, 1000), kLisbonCell);
  CHECK_EQ(inspire_cell_of(kLisbonNearSE, 1000), kLisbonCell);
  CHECK_EQ(inspire_cell_of(kLisbonNearNW, 1000), kLisbonCell);
  CHECK_EQ(inspire_cell_of(kLisbonOutsideSW, 1000),
           inspire_cell{
               .northing_ = 1944000, .easting_ = 2664000, .resolution_ = 1000});

  // near SE corner: 10 m north of the cell's southern edge, 990 m east of its
  // western edge
  CHECK_EQ(inspire_cell_of(kLisbonNearSE, 100),
           inspire_cell{
               .northing_ = 1945000, .easting_ = 2665900, .resolution_ = 100});

  CHECK_THROWS(inspire_cell_of(kLisbonCenter, 0));
}

TEST_CASE("etrs89_laea") {
  // with a resolution of 1 m, a cell's origin is the position projected to
  // ETRS89-LAEA, rounded down to whole meters; expected values from PROJ
  auto const meter_cell = [](std::int64_t const northing,
                             std::int64_t const easting) {
    return inspire_cell{
        .northing_ = northing, .easting_ = easting, .resolution_ = 1};
  };

  // Lisboa, Sulina, Hornstrandir, Utsjoki, Valletta
  CHECK_EQ(inspire_cell_of({38.707482, -9.13702}, 1),
           meter_cell(1945115, 2665342));
  CHECK_EQ(inspire_cell_of({45.155896, 29.654932}, 1),
           meter_cell(2652428, 5848670));
  CHECK_EQ(inspire_cell_of({66.362433, -22.443008}, 1),
           meter_cell(5141803, 2920103));
  CHECK_EQ(inspire_cell_of({69.907579, 27.025337}, 1),
           meter_cell(5278479, 4974178));
  CHECK_EQ(inspire_cell_of({35.899480, 14.513626}, 1),
           meter_cell(1438675, 4732145));

  // outside the grid's extent, negative northing and easting: Saint-Denis
  // (Réunion), Fort-de-France (Martinique)
  CHECK_EQ(inspire_cell_of({-20.88, 55.45}, 1), meter_cell(-3030127, 9981166));
  CHECK_EQ(inspire_cell_of({14.60, -61.07}, 1), meter_cell(2507054, -2676004));
}

TEST_CASE("population_grid") {
  auto const g = parse_eurostat_population_grid(kGridCSV);

  REQUIRE_EQ(g.size(), 5U);

  CHECK_EQ(g[0].id_,
           inspire_cell{
               .northing_ = 2683000, .easting_ = 4285000, .resolution_ = 1000});
  CHECK_EQ(g[0].data_, 0U);

  CHECK_EQ(g[1].id_,
           inspire_cell{
               .northing_ = 2684000, .easting_ = 4285000, .resolution_ = 1000});
  CHECK_EQ(g[1].data_, 25U);

  CHECK_EQ(g[2].id_,
           inspire_cell{
               .northing_ = 2682000, .easting_ = 4286000, .resolution_ = 1000});
  CHECK_EQ(g[2].data_, 0U);

  CHECK_EQ(g[3].id_,
           inspire_cell{
               .northing_ = 2683000, .easting_ = 4286000, .resolution_ = 1000});
  CHECK_EQ(g[3].data_, 116U);

  CHECK_EQ(g[4].id_,
           inspire_cell{
               .northing_ = 2684000, .easting_ = 4286000, .resolution_ = 1000});
  CHECK_EQ(g[4].data_, 185U);
}

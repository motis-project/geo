#include "catch.hpp"

#include "geo/polyline.h"
#include "geo/simplify.h"
#include "geo/webmercator.h"

TEST_CASE("simplification_mask polyline (simple)") {
  using proj = geo::webmercator<4096>;

  SECTION("all required") {
    geo::polyline in{geo::merc_to_latlng(proj::pixel_to_merc({0, 0}, 0)),
                     geo::merc_to_latlng(proj::pixel_to_merc({50, 0}, 0)),
                     geo::merc_to_latlng(proj::pixel_to_merc({100, 0}, 0))};

    auto const out = simplification_mask(in);
    REQUIRE(out.size() == 21);
    REQUIRE(out[0].size() == 3);

    CHECK(out[0][0] == true);
    CHECK(out[0][1] == false);
    CHECK(out[0][2] == true);
  }

  SECTION("slight deviation") {
    geo::polyline in{geo::merc_to_latlng(proj::pixel_to_merc({0, 0}, 0)),
                     geo::merc_to_latlng(proj::pixel_to_merc({50, 1}, 0)),
                     geo::merc_to_latlng(proj::pixel_to_merc({100, 0}, 0))};

    auto const out = simplification_mask(in);
    REQUIRE(out.size() == 21);
    REQUIRE(out[0].size() == 3);

    for (auto i = 0; i < 3; ++i) {
      CAPTURE(i);
      CHECK(out[0][i] == true);
    }

    auto const out2 = simplification_mask(in, 2);
    REQUIRE(out2.size() == 21);
    REQUIRE(out2[0].size() == 3);

    CHECK(out2[0][0] == true);
    CHECK(out2[0][1] == false);
    CHECK(out2[0][2] == true);
  }

  SECTION("another level") {
    geo::polyline in{geo::merc_to_latlng(proj::pixel_to_merc({20, 0}, 10)),
                     geo::merc_to_latlng(proj::pixel_to_merc({21, 25}, 10)),
                     geo::merc_to_latlng(proj::pixel_to_merc({20, 50}, 10))};

    auto const out = simplification_mask(in);
    REQUIRE(out.size() == 21);
    REQUIRE(out[0].size() == 3);

    for(auto z = 0; z < 10; ++z) {
      
    for (auto i = 0; i < 3; ++i) {
      CAPTURE(i);
      CHECK(out[0][i] == true);
    }
    }



  }
}

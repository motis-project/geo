#include "catch.hpp"

#include "geo/polyline.h"
#include "geo/simplify_mask.h"
#include "geo/webmercator.h"

TEST_CASE("make_simplify_mask") {
  using proj = geo::webmercator<4096>;

  auto const px2ll = [](auto const x, auto const y, auto const z) {
    return geo::merc_to_latlng(proj::pixel_to_merc({x, y}, z));
  };

  SECTION("all required") {
    geo::polyline in{px2ll(0, 0, 0), px2ll(50, 0, 0), px2ll(100, 0, 0)};

    auto const out = make_simplify_mask(in);
    REQUIRE(out.size() == 21);
    REQUIRE(out[0].size() == 3);

    CHECK(out[0][0] == true);
    CHECK(out[0][1] == false);
    CHECK(out[0][2] == true);
  }
  SECTION("slight deviation") {
    geo::polyline in{px2ll(0, 0, 0), px2ll(50, 1, 0), px2ll(100, 0, 0)};

    auto const out = make_simplify_mask(in);
    REQUIRE(out.size() == 21);
    REQUIRE(out[0].size() == 3);

    for (auto i = 0; i < 3; ++i) {
      CAPTURE(i);
      CHECK(out[0][i] == true);
    }

    auto const out2 = make_simplify_mask(in, 2);
    REQUIRE(out2.size() == 21);
    REQUIRE(out2[0].size() == 3);

    CHECK(out2[0][0] == true);
    CHECK(out2[0][1] == false);
    CHECK(out2[0][2] == true);
  }

  SECTION("recursion") {
    geo::polyline in{px2ll(0, 0, 0), px2ll(50, 1, 0), px2ll(100, 0, 0),
                     px2ll(100, 100, 0)};

    auto const out = make_simplify_mask(in, 2);
    REQUIRE(out.size() == 21);

    REQUIRE(out[0].size() == 4);
    CHECK(out[0][0] == true);
    CHECK(out[0][1] == false);
    CHECK(out[0][2] == true);
    CHECK(out[0][3] == true);

    for (auto z = 1; z < 21; ++z) {
      CAPTURE(z);
      REQUIRE(out[z].size() == 4);
      CHECK(out[z][0] == true);
      CHECK(out[z][1] == true);
      CHECK(out[z][2] == true);
      CHECK(out[z][3] == true);
    }
  }

  SECTION("mid level") {
    geo::polyline in{px2ll(20, 0, 10), px2ll(21, 25, 10), px2ll(20, 50, 10)};

    auto const out = make_simplify_mask(in);
    REQUIRE(out.size() == 21);

    for (auto z = 0; z <= 10; ++z) {
      CAPTURE(z);
      REQUIRE(out[z].size() == 3);
      CHECK(out[z][0] == true);
      CHECK(out[z][1] == false);
      CHECK(out[z][2] == true);
    }

    for (auto z = 11; z < 21; ++z) {
      CAPTURE(z);
      REQUIRE(out[z].size() == 3);
      for (auto i = 0; i < 3; ++i) {
        CAPTURE(i);
        CHECK(out[z][i] == true);
      }
    }
  }
}

TEST_CASE("apply_simplify_mask") {
  std::vector<int> vec{0, 1, 2, 3};

  {
    auto uut = vec;
    geo::apply_simplify_mask({true, true, true, true}, uut);
    REQUIRE(vec == uut);
  }

  {
    auto uut = vec;
    std::vector<int> expected{0, 3};
    geo::apply_simplify_mask({true, false, false, true}, uut);
    REQUIRE(expected == uut);
  }

  {
    auto uut = vec;
    std::vector<int> expected{0, 2, 3};
    geo::apply_simplify_mask({true, false, true, true}, uut);
    REQUIRE(expected == uut);
  }

  {
    std::vector<int> uut{0, 1, 2, 3, 4, 5, 6};
    std::vector<int> expected{0, 5, 6};
    geo::apply_simplify_mask({true, false, false, false, false, true, true},
                             uut);
    REQUIRE(expected == uut);
  }

  {
    std::vector<int> uut{0, 1, 2, 3, 4, 5, 6};
    std::vector<int> expected{0, 2, 3, 6};
    geo::apply_simplify_mask({true, false, true, true, false, false, true},
                             uut);
    REQUIRE(expected == uut);
  }
}
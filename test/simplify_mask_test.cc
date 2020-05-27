#include "catch2/catch.hpp"

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

TEST_CASE("simplify_mask_serialize") {
  auto const get_lvls = [](auto const& str) -> uint32_t {
    uint32_t r;
    std::memcpy(&r, str.data(), sizeof(uint32_t));
    return r;
  };
  auto const get_size = [](auto const& str) -> uint32_t {
    uint32_t r;
    std::memcpy(&r, str.data() + sizeof(uint32_t), sizeof(uint32_t));
    return r;
  };
  auto const get_data = [](auto const& str, int i = 0) -> char {
    return *(str.data() + 2 * sizeof(uint32_t) + i);
  };

  SECTION("simple") {
    std::vector<std::vector<bool>> sut{{true, true}};

    auto str = geo::serialize_simplify_mask(sut);

    REQUIRE(str.size() == 2 * sizeof(uint32_t) + sizeof(char));

    REQUIRE(get_lvls(str) == 0x1);
    REQUIRE(get_size(str) == 0x2);
    REQUIRE(get_data(str) == 0x3);
  }

  SECTION("skip") {
    std::vector<std::vector<bool>> sut{{true, true}, {true, true}};

    auto str = geo::serialize_simplify_mask(sut);

    REQUIRE(str.size() == 2 * sizeof(uint32_t) + sizeof(char));

    REQUIRE(get_lvls(str) == 0x2);
    REQUIRE(get_size(str) == 0x2);
    REQUIRE(get_data(str) == 0x3);
  }

  SECTION("multibyte") {
    std::vector<std::vector<bool>> sut{
        {true, true, true, true, true, true, true, true, false, true}};

    auto str = geo::serialize_simplify_mask(sut);

    REQUIRE(str.size() == 2 * sizeof(uint32_t) + 2 * sizeof(char));

    REQUIRE(get_lvls(str) == 0x1);
    REQUIRE(get_size(str) == 10);

    // catch problem!?
    REQUIRE(static_cast<char>(get_data(str, 0)) == static_cast<char>(0xFF));
    REQUIRE(static_cast<char>(get_data(str, 1)) == static_cast<char>(0x2));
  }
}

TEST_CASE("simplify_mask_serial_apply") {
  SECTION("simple") {
    std::vector<std::vector<bool>> mask{{true, false, true}};
    auto mask_str = geo::serialize_simplify_mask(mask);

    std::vector<int> sut{1, 2, 3};
    geo::apply_simplify_mask(mask_str, 0, sut);

    REQUIRE(sut == (std::vector<int>{1, 3}));
  }

  SECTION("complex") {
    std::vector<std::vector<bool>> mask{
        {true, false, false, false, false, false, true, false, false, true},
        {true, false, true, true, false, false, true, false, false, true},
        {true, false, true, true, false, false, true, false, false, true}};

    auto mask_str = geo::serialize_simplify_mask(mask);

    REQUIRE(mask_str.size() == 2 * sizeof(uint32_t) + 3 * sizeof(char));

    std::vector<int> sut0{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    geo::apply_simplify_mask(mask_str, 0, sut0);
    REQUIRE(sut0 == (std::vector<int>{0, 6, 9}));

    std::vector<int> sut1{0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    geo::apply_simplify_mask(mask_str, 1, sut1);
    REQUIRE(sut1 == (std::vector<int>{0, 2, 3, 6, 9}));
  }
}

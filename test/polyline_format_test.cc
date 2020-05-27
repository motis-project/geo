#include "catch.hpp"

#include "geo/polyline_format.h"

// the official example from :
// https://developers.google.com/maps/documentation/utilities/polylinealgorithm
TEST_CASE("polyline_format_google_coord") {

  geo::polyline_encoder<> enc;
  enc.push_difference(-179.9832104 * geo::polyline_encoder<>::kPrecision);
  CHECK("`~oia@" == enc.buf_);

  auto const line = geo::decode_polyline("`~oia@");
  REQUIRE(1 == line.size());
  CHECK(-179.98321 == line[0].lat_);
  CHECK(0 == line[0].lng_);
}

TEST_CASE("path_polyline_format_test_google_polyline") {
  geo::polyline original{{38.5, -120.2}, {40.7, -120.95}, {43.252, -126.453}};

  auto const encoded = geo::encode_polyline(original);
  CHECK("_p~iF~ps|U_ulLnnqC_mqNvxq`@" == encoded);

  auto const decoded = geo::decode_polyline(encoded);
  CHECK(original == decoded);
}

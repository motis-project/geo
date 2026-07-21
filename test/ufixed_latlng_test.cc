#include "doctest/doctest.h"

#include "geo/ufixed_latlng.h"

TEST_CASE("shift_prime_meridian") {
  CHECK(geo::ufixed_latlng::shift_prime_meridian(-168.5) == 0.0);
  CHECK(geo::ufixed_latlng::shift_prime_meridian(0.0) == 168.5);
  CHECK(geo::ufixed_latlng::shift_prime_meridian(-173) == 355.5);
}

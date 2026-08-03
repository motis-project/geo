#include "etrs89_laea.h"

#include <cmath>

#include "geo/rad_deg.h"

namespace geo {

// https://inspire-mif.github.io/technical-guidelines/data/gg/dataspecification_gg.pdf


static constexpr auto a  = 6378137.0;
static constexpr auto kF = 1 / 298.257222101;

static constexpr auto kE2 = 2 * kF - kF * kF;
static auto const kE = std::sqrt(kE2);

static constexpr auto lat0_deg = 52.0;   // latitude of origin
static constexpr auto lon0_deg = 10.0;   // longitude of origin
static constexpr auto FE = 4321000.0;    // false easting
static constexpr auto FN = 3210000.0;    // false northing

static constexpr auto phi0 = to_rad(lat0_deg);

double q (const double phi) {
  auto const sin_phi = std::sin(phi);
  return (1.0 - kE2) * (sin_phi / (1.0 - kE2 * sin_phi * sin_phi)
    - 1.0 / (2.0 * kE));
}

double authalic_to_geodetic()

latlng from_etrs89_laea(std::uint32_t northing, std::uint32_t easting) {

  auto const q = [](auto const phi) {

  };

}

} // namespace geo
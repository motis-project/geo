#include "geo/grid.h"

#include <cctype>
#include <charconv>
#include <cmath>
#include <utility>

#include "utl/parser/csv_range.h"
#include "utl/pipes/for_each.h"
#include "utl/verify.h"

#include "geo/constants.h"
#include "geo/rad_deg.h"

namespace geo {

constexpr auto kEtrs89Laea = "EPSG:3035";
constexpr auto kEtrs89LaeaCode = "3035";
constexpr auto kEtrs89LaeaName = "ETRS89-LAEA";

// ETRS89-LAEA: Lambert azimuthal equal-area projection of the GRS80 ellipsoid
// centered at 52°N 10°E, false northing 3210000 m, false easting 4321000 m
// formulas: IOGP Guidance Note 7-2, method 9820 (Lambert Azimuthal Equal Area)
// returns {northing, easting} in meters
inline std::pair<double, double> to_etrs89_laea(latlng const& pos) {
  constexpr auto kA = 6378137.0;  // GRS80 semi-major axis
  constexpr auto kF = 1.0 / 298.257222101;  // GRS80 flattening
  constexpr auto kE2 = kF * (2.0 - kF);  // squared eccentricity
  constexpr auto kLat0 = to_rad(52.0);
  constexpr auto kLng0 = to_rad(10.0);
  constexpr auto kFalseNorthing = 3210000.0;
  constexpr auto kFalseEasting = 4321000.0;

  static auto const e = std::sqrt(kE2);
  auto const q = [](double const lat) {
    auto const s = std::sin(lat);
    return (1.0 - kE2) * (s / (1.0 - kE2 * s * s) -
                          std::log((1.0 - e * s) / (1.0 + e * s)) / (2.0 * e));
  };
  static auto const q_p = q(kPI / 2.0);
  static auto const beta0 = std::asin(q(kLat0) / q_p);
  static auto const r_q = kA * std::sqrt(q_p / 2.0);
  static auto const d =
      kA * std::cos(kLat0) /
      std::sqrt(1.0 - kE2 * std::sin(kLat0) * std::sin(kLat0)) /
      (r_q * std::cos(beta0));

  auto const beta = std::asin(q(to_rad(pos.lat_)) / q_p);
  auto const dlng = to_rad(pos.lng_) - kLng0;
  auto const b =
      r_q *
      std::sqrt(2.0 / (1.0 + std::sin(beta0) * std::sin(beta) +
                       std::cos(beta0) * std::cos(beta) * std::cos(dlng)));
  return {
      kFalseNorthing + b / d *
                           (std::cos(beta0) * std::sin(beta) -
                            std::sin(beta0) * std::cos(beta) * std::cos(dlng)),
      kFalseEasting + b * d * std::cos(beta) * std::sin(dlng)};
}

inspire_cell parse_inspire_grid_id(std::string_view const s) {
  auto pos = 0U;

  auto const expect = [&](std::string_view const tag) {
    utl::verify(s.substr(pos, tag.size()) == tag,
                "parse_inspire_grid_id: expected \"{}\" at position {} in "
                "\"{}\"",
                tag, pos, s);
    pos += tag.size();
  };

  auto const parse_digits = [&]() -> std::int64_t {
    auto const start = pos;
    while (pos < s.size() &&
           std::isdigit(static_cast<unsigned char>(s[pos])) != 0) {
      ++pos;
    }
    utl::verify(pos > start,
                "parse_inspire_grid_id: expected number at position {} in "
                "\"{}\"",
                start, s);
    auto value = std::int64_t{0};
    auto const [ptr, ec] =
        std::from_chars(s.data() + start, s.data() + pos, value);
    utl::verify(ec == std::errc{},
                "parse_inspire_grid_id: invalid number in \"{}\"", s);
    return value;
  };

  expect("CRS");
  auto const epsg_start = pos;
  while (pos < s.size() &&
         std::isdigit(static_cast<unsigned char>(s[pos])) != 0) {
    ++pos;
  }
  utl::verify(pos > epsg_start,
              "parse_inspire_grid_id: expected EPSG code in \"{}\"", s);
  auto const epsg_code = s.substr(epsg_start, pos - epsg_start);
  utl::verify(epsg_code == kEtrs89LaeaCode,
              "parse_inspire_grid_id: unsupported CRS EPSG:{} in \"{}\", "
              "only {} {} is supported",
              epsg_code, s, kEtrs89Laea, kEtrs89LaeaName);

  expect("RES");
  auto const resolution = parse_digits();
  expect("m");

  expect("N");
  auto const northing = parse_digits();

  expect("E");
  auto const easting = parse_digits();

  return {
      .northing_ = northing, .easting_ = easting, .resolution_ = resolution};
}

inspire_cell inspire_cell_of(latlng const& pos, std::int64_t const resolution) {
  utl::verify(resolution > 0,
              "inspire_cell_of: resolution must be positive, got {}",
              resolution);
  auto const [northing, easting] = to_etrs89_laea(pos);
  utl::verify(std::isfinite(northing) && std::isfinite(easting),
              "inspire_cell_of: can not project {} to {}", pos, kEtrs89Laea);
  auto const to_cell_origin = [&](double const x) {
    return static_cast<std::int64_t>(
               std::floor(x / static_cast<double>(resolution))) *
           resolution;
  };
  return {.northing_ = to_cell_origin(northing),
          .easting_ = to_cell_origin(easting),
          .resolution_ = resolution};
}

grid<std::uint64_t> parse_eurostat_population_grid(std::string_view const csv) {
  struct csv_grid_cell {
    utl::csv_col<utl::cstr, UTL_NAME("GRD_ID")> id_;
    utl::csv_col<std::uint64_t, UTL_NAME("T")> total_population_;
  };

  auto g = grid<std::uint64_t>{};
  utl::line_range{utl::make_buf_reader(csv)} | utl::csv<csv_grid_cell>() |
      utl::for_each([&](csv_grid_cell const& c) {
        try {
          g.emplace_back(parse_inspire_grid_id(c.id_->trim().view()),
                         *c.total_population_);
        } catch (std::exception const& e) {
          fmt::println("could not parse grid cell: {}, {}, {}",
                       c.id_->trim().view(), *c.total_population_, e.what());
        }
      });
  return g;
}

}  // namespace geo

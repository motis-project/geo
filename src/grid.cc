#include "geo/grid.h"

#include <cctype>
#include <charconv>

#include "utl/parser/csv_range.h"
#include "utl/verify.h"

#include "geo/proj_transformers.h"

namespace geo {

constexpr auto kEtrs89Laea = "EPSG:3035";
constexpr auto kEtrs89LaeaCode = "3035";
constexpr auto kEtrs89LaeaName = "ETRS89-LAEA";

box parse_inspire_grid_id(std::string_view const s) {
  auto pos = 0U;

  auto const expect = [&](std::string_view const tag) {
    utl::verify(s.substr(pos, tag.size()) == tag,
                "parse_inspire_grid_id: expected \"{}\" at position {} in "
                "\"{}\"",
                tag, pos, s);
    pos += tag.size();
  };

  auto const parse_uint = [&]() -> std::uint64_t {
    auto const start = pos;
    while (pos < s.size() &&
           std::isdigit(static_cast<unsigned char>(s[pos])) != 0) {
      ++pos;
    }
    utl::verify(pos > start,
                "parse_inspire_grid_id: expected number at position {} in "
                "\"{}\"",
                start, s);
    auto value = std::uint64_t{0U};
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
  auto const resolution = parse_uint();
  expect("m");

  expect("N");
  auto const northing = parse_uint();

  expect("E");
  auto const easting = parse_uint();

  static auto transformers = proj_transformers{};
  auto const sw = transformers.transform(
      kEtrs89Laea, static_cast<double>(northing), static_cast<double>(easting));
  auto const ne = transformers.transform(
      kEtrs89Laea, static_cast<double>(northing + resolution),
      static_cast<double>(easting + resolution));

  return box{sw, ne};
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
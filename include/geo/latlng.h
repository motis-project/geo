#pragma once

#include <cmath>
#include <cstdint>
#include <array>
#include <iosfwd>
#include <limits>
#include <tuple>

namespace geo {

struct latlng {
  double lat() const noexcept { return lat_; }
  double lng() const noexcept { return lng_; }

  friend std::ostream& operator<<(std::ostream&, latlng const&);

  friend bool operator<(latlng const& lhs, latlng const& rhs) noexcept {
    return std::tie(lhs.lat_, lhs.lng_) < std::tie(rhs.lat_, rhs.lng_);
  }

  friend bool operator==(latlng const& lhs, latlng const& rhs) noexcept {
    auto const lat_diff = std::abs(lhs.lat_ - rhs.lat_);
    auto const lng_diff = std::abs(lhs.lng_ - rhs.lng_);
    return lat_diff < 100 * std::numeric_limits<double>::epsilon() &&
           lng_diff < 100 * std::numeric_limits<double>::epsilon();
  }

  std::array<double, 2> lnglat() const noexcept { return {lng_, lat_}; }

  double lat_{0.0}, lng_{0.0};
};

double distance(latlng const&, latlng const&);

double bearing(latlng const&, latlng const&);

latlng midpoint(latlng const&, latlng const&);

latlng closest_on_segment(latlng const& x, latlng const& segment_from,
                          latlng const& segment_to);

latlng destination_point(latlng const& source, double const distance,
                         double const bearing);

uint32_t tile_hash_32(latlng const&);

}  // namespace geo

#if __has_include("fmt/format.h")

#include "fmt/format.h"

template <>
struct fmt::formatter<geo::latlng> : nested_formatter<double> {
  auto format(geo::latlng const& p, format_context& ctx) const {
    return write_padded(ctx, [&](auto out) {
      return format_to(out, "({}, {})", nested(p.lat_), nested(p.lng_));
    });
  }
};

#endif
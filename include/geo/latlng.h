#pragma once

#include <cstdint>
#include <iosfwd>
#include <tuple>

namespace geo {

struct latlng {
  double lat() const { return lat_; }
  double lng() const { return lng_; }

  friend std::ostream& operator<<(std::ostream&, latlng const&);

  friend bool operator<(latlng const& lhs, latlng const& rhs) {
    return std::tie(lhs.lat_, lhs.lng_) < std::tie(rhs.lat_, rhs.lng_);
  }

  friend bool operator==(latlng const& lhs, latlng const& rhs) {
    return std::tie(lhs.lat_, lhs.lng_) == std::tie(rhs.lat_, rhs.lng_);
  }

  double lat_{0.0}, lng_{0.0};
};

double distance(latlng const&, latlng const&);

double bearing(latlng const&, latlng const&);

latlng midpoint(latlng const&, latlng const&);

latlng closest_on_segment(latlng const& x, latlng const& segment_from,
                          latlng const& segment_to);

latlng destination_point(latlng const& source, double const distance, double const bearing);

uint32_t tile_hash_32(latlng const&);

}  // namespace geo

#if __has_include("fmt/ostream.h")

#include "fmt/ostream.h"

template <>
struct fmt::formatter<geo::latlng> : ostream_formatter {};

#endif
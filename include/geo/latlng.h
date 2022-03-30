#pragma once

#include <cstdint>
#include <iosfwd>
#include <tuple>

namespace geo {

struct latlng {
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

uint32_t tile_hash_32(latlng const&);

}  // namespace geo

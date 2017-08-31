#pragma once

#include <cstdint>
#include <vector>

#include "geo/latlng.h"
#include "geo/polyline.h"

namespace geo {

using simplify_mask_t = std::vector<std::vector<bool>>;

simplify_mask_t make_simplify_mask(polyline const&,
                                   uint32_t const pixel_precision = 1);

template <typename T>
void apply_simplify_mask(std::vector<bool> const& mask, std::vector<T>& vec) {
  assert(mask.size() == vec.size());
  if (mask.empty()) {
    return;
  }
  assert(mask.at(0) == true);

  auto first = std::next(begin(vec));
  size_t pos = 1;
  for (auto it = first; it != end(vec); ++pos, ++it) {
    if (mask[pos]) {
      *first++ = std::move(*it);
    }
  }

  vec.erase(first, end(vec));
}

}  // namespace geo

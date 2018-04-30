#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "geo/latlng.h"
#include "geo/polyline.h"

namespace geo {

constexpr auto kMaxSimplifyZoomLevel = 20;
constexpr auto kSimplifyZoomLevels = kMaxSimplifyZoomLevel + 1;
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

std::string serialize_simplify_mask(simplify_mask_t const&);

template <typename T>
void apply_simplify_mask(std::string const& mask, int req_lvl,
                         std::vector<T>& vec) {
  assert(req_lvl >= 0 && req_lvl <= kMaxSimplifyZoomLevel);

  uint32_t lvls = *reinterpret_cast<uint32_t const*>(mask.data());
  assert(lvls != 0);

  uint32_t size =
      *reinterpret_cast<uint32_t const*>(mask.data() + sizeof(uint32_t));

  assert(size == vec.size());
  if (size == 0) {
    return;
  }

  auto skipped_levels = 0;
  for (auto i = 0; i < 32; ++i) {
    if (i >= req_lvl) {
      break;
    }
    if ((lvls & (1u << i)) != 0) {
      ++skipped_levels;
    }
  }
  assert(lvls >= (1u << skipped_levels));

  auto base_ptr = mask.data() + 2 * sizeof(uint32_t);
  auto offset = skipped_levels * size;

  auto get_bit = [&base_ptr, &offset](size_t const pos) -> bool {
    auto byte = *(base_ptr + (offset + pos) / 8);
    return (byte >> (offset + pos) % 8) & 0x1;
  };

  assert(get_bit(0) == true);

  auto first = std::next(begin(vec));
  size_t pos = 1;
  for (auto it = first; it != end(vec); ++pos, ++it) {
    if (get_bit(pos)) {
      *first++ = std::move(*it);
    }
  }

  vec.erase(first, end(vec));
}

}  // namespace geo

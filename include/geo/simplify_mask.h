#pragma once

#include <cstdint>
#include <vector>

#include "geo/latlng.h"
#include "geo/polyline.h"

namespace geo {

using simplify_mask_t = std::vector<std::vector<bool>>;

simplify_mask_t make_mask(polyline const&, uint32_t const pixel_precision = 1);

}  // namespace geo

#pragma once

#include <math.h>
#include <iostream>

// #include <boost/assert.hpp>

#include "geo/constants.h"
#include "geo/latlng.h"
#include "geo/polyline.h"
#include "geo/webmercator.h"

namespace geo {

using zoom_level_t = uint32_t;
using simplify_mask_t = std::vector<std::vector<bool>>;

simplify_mask_t simplification_mask(polyline const&,
                                    uint32_t const pixel_precision = 1);

}  // namespace geo

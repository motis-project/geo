#pragma once

#include <string>

#include "geo/fixed/fixed_geometry.h"

namespace geo {

fixed_geometry deserialize(std::string_view geo);
fixed_geometry deserialize(std::string_view geo,
                           std::vector<std::string_view> simplify_masks,
                           uint32_t z);

}  // namespace geo

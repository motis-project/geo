#pragma once

#include "geo/fixed/fixed_geometry.h"

namespace geo {

std::string serialize(fixed_point const&);
std::string serialize(fixed_polyline const&);
std::string serialize(fixed_polygon const&);
std::string serialize(fixed_geometry const&);

}  // namespace geo

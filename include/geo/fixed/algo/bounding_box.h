#pragma once

#include "geo/fixed/fixed_geometry.h"

namespace geo {

fixed_box bounding_box(fixed_point const&);
fixed_box bounding_box(fixed_polyline const&);
fixed_box bounding_box(fixed_polygon const&);
fixed_box bounding_box(fixed_simple_polygon const&);
fixed_box bounding_box(fixed_geometry const&);

}  // namespace geo

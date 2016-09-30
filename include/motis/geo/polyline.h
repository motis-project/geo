#pragma once

#include <vector>

#include "motis/geo/latlng.h"

namespace motis {
namespace geo {

using polyline = std::vector<latlng>;

double length(polyline const&);

polyline simplify(polyline const&);

}  // namespace geo
}  // namespace motis

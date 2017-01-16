#pragma once

#include <vector>

#include "geo/latlng.h"

namespace geo {

using polyline = std::vector<latlng>;

double length(polyline const&);

polyline simplify(polyline const&);

polyline extract(polyline const&, size_t const from, size_t const to);

}  // namespace geo

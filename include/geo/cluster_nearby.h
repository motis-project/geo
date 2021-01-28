#pragma once

#include <limits>
#include <vector>

#include "geo/latlng.h"

namespace geo {

using cluster_id_t = uint32_t;
constexpr cluster_id_t NO_CLUSTER = std::numeric_limits<cluster_id_t>::max();

std::vector<cluster_id_t> cluster_nearby(std::vector<latlng> const& coords,
                                         float max_dist);

}  // namespace geo

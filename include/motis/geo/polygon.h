#pragma once

#include <vector>

#include "motis/geo/latlng.h"

namespace motis {
namespace geo {

using simple_polygon = std::vector<latlng>;

simple_polygon read_poly_file(std::string const& filename);

bool within(latlng const&, simple_polygon const&);

}  // namespace geo
}  // namespace motis

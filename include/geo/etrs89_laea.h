#pragma once

#include "latlng.h"

namespace geo {

latlng from_etrs89_laea(std::uint32_t easting, std::uint32_t northing);

} // namespace geo
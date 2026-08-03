#pragma once

#include "latlng.h"

namespace geo {

latlng from_etrs89_laea(std::uint32_t northing, std::uint32_t easting);

} // namespace geo
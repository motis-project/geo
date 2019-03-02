#pragma once

#include "boost/geometry.hpp"
#include "boost/geometry/geometries/register/box.hpp"

#include "geo/box.h"
#include "geo/latlng.h"

#include "geo/detail/register_latlng.h"

BOOST_GEOMETRY_REGISTER_BOX(geo::box, geo::latlng, min_, max_);

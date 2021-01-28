#include "geo/polygon.h"

#include <cmath>
#include <fstream>
#include <istream>
#include <string>

#include "boost/geometry.hpp"

#include "geo/detail/register_latlng.h"
#include "geo/detail/register_polygon.h"

namespace geo {

simple_polygon read_poly_file(std::string const& filename) {
  std::ifstream in(filename);
  in.exceptions(std::ios::failbit);

  std::string ignored;
  std::getline(in, ignored);
  std::getline(in, ignored);

  simple_polygon polygon;
  while (!in.eof()) {
    if (in.peek() != ' ' || in.peek() == '\n' || in.eof()) {
      break;
    }

    double lat = NAN, lng = NAN;
    in >> lng >> lat;

    polygon.emplace_back(lat, lng);
    std::getline(in, ignored);
  }
  return polygon;
}

bool within(latlng const& point, simple_polygon const& polygon) {
  return boost::geometry::within(point, polygon);
}

}  // namespace geo

#include "motis/geo/polygon.h"

#include <fstream>

#include "boost/geometry.hpp"

#include "motis/geo/detail/register_latlng.h"
#include "motis/geo/detail/register_polygon.h"

namespace motis {
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

    double lat, lng;
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
}  // namespace motis

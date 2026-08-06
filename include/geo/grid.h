#pragma once

#include "geo/box.h"

namespace geo {

template <typename T>
struct grid_cell {
  box b_;
  T data_;
};

template <typename T>
using grid = std::vector<grid_cell<T>>;

box parse_inspire_grid_id(std::string_view);

grid<std::uint64_t> read_eurostat_population_grid(std::string_view csv);

} // namespace geo
#pragma once

#include <memory>
#include <vector>

#include "geo/box.h"
#include "geo/latlng.h"

namespace geo {

struct point_rtree {
  using value_t = std::pair<latlng, size_t>;

  point_rtree();
  ~point_rtree();

  explicit point_rtree(std::vector<value_t> const&);
  point_rtree(point_rtree&&) noexcept;
  point_rtree& operator=(point_rtree&&) noexcept;

  point_rtree(point_rtree const&) = delete;
  point_rtree& operator=(point_rtree const&) = delete;

  std::vector<std::pair<double, size_t>> in_radius_with_distance(
      latlng const& center, double min_radius, double max_radius) const;

  std::vector<std::pair<double, size_t>> nearest(latlng const& center,
                                                 unsigned) const;

  std::vector<std::pair<double, size_t>> in_radius_with_distance(
      latlng const& center, double max_radius) const;

  std::vector<size_t> in_radius(latlng const& center, double min_radius,
                                double max_radius) const;

  std::vector<size_t> in_radius(latlng const& center, double max_radius) const;

  std::vector<size_t> within(geo::box const&) const;

  std::size_t size() const;

private:
  struct impl;
  std::unique_ptr<impl> impl_;
};

template <typename C, typename F>
point_rtree make_point_rtree(C&& container, F fun) {
  auto i = 0;
  std::vector<point_rtree::value_t> index;
  index.reserve(container.size());
  for (auto const& e : container) {
    index.emplace_back(fun(e), i++);
  }
  return point_rtree{index};
}

template <typename C>
point_rtree make_point_rtree(C const& container) {
  auto i = 0;
  std::vector<point_rtree::value_t> index;
  for (auto const& e : container) {
    index.emplace_back(e, i++);
  }
  return point_rtree{index};
}

}  // namespace geo

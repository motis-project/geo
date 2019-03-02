#include "geo/point_rtree.h"

#include "boost/function_output_iterator.hpp"
#include "boost/geometry/geometries/box.hpp"
#include "boost/geometry/geometries/point.hpp"
#include "boost/geometry/index/rtree.hpp"

#include "geo/detail/register_box.h"
#include "geo/detail/register_latlng.h"

namespace bgi = boost::geometry::index;

namespace geo {

struct point_rtree::impl {
  using rtree_t = bgi::rtree<value_t, bgi::quadratic<16>>;

  impl() = default;
  explicit impl(std::vector<value_t> const& index) : rtree_(index) {}

  std::vector<std::pair<double, size_t>> in_radius_with_distance(
      latlng const& center, double const min_radius,
      double const max_radius) const {
    std::vector<std::pair<double, size_t>> results;
    rtree_.query(bgi::intersects(box{center, max_radius}),
                 boost::make_function_output_iterator([&](auto&& v) {
                   auto const dist = distance(v.first, center);
                   if (dist >= max_radius || dist < min_radius) {
                     return;
                   }
                   results.emplace_back(dist, v.second);
                 }));

    std::sort(begin(results), end(results));
    return results;
  }

  std::vector<std::pair<double, size_t>> in_radius_with_distance(
      latlng const& center, double const max_radius) const {
    return in_radius_with_distance(center, 0, max_radius);
  }

  std::vector<size_t> in_radius(latlng const& center, double const min_radius,
                                double const max_radius) const {
    std::vector<size_t> result;
    for (auto const& pair :
         in_radius_with_distance(center, min_radius, max_radius)) {
      result.emplace_back(pair.second);
    }
    return result;
  }

  std::vector<size_t> in_radius(latlng const& center,
                                double const max_radius) const {
    return in_radius(center, 0, max_radius);
  }

  std::vector<size_t> within(geo::box const& box) const {
    std::vector<size_t> results;
    rtree_.query(bgi::intersects(box),
                 boost::make_function_output_iterator(
                     [&](auto&& v) { results.emplace_back(v.second); }));

    std::sort(begin(results), end(results));
    return results;
  }

  rtree_t rtree_;
};

point_rtree::point_rtree() : impl_(std::make_unique<point_rtree::impl>()) {}
point_rtree::~point_rtree() = default;

point_rtree::point_rtree(std::vector<value_t> const& index)
    : impl_(std::make_unique<point_rtree::impl>(index)) {}
point_rtree::point_rtree(point_rtree&&) = default;
point_rtree& point_rtree::operator=(point_rtree&&) = default;

std::vector<std::pair<double, size_t>> point_rtree::in_radius_with_distance(
    latlng const& center, double const min_radius,
    double const max_radius) const {
  return impl_->in_radius_with_distance(center, min_radius, max_radius);
}

std::vector<std::pair<double, size_t>> point_rtree::in_radius_with_distance(
    latlng const& center, double const max_radius) const {
  return impl_->in_radius_with_distance(center, max_radius);
}

std::vector<size_t> point_rtree::in_radius(latlng const& center,
                                           double const min_radius,
                                           double const max_radius) const {
  return impl_->in_radius(center, min_radius, max_radius);
}

std::vector<size_t> point_rtree::in_radius(latlng const& center,
                                           double const max_radius) const {
  return impl_->in_radius(center, max_radius);
}

std::vector<size_t> point_rtree::within(geo::box const& box_) const {
  return impl_->within(box_);
}

}  // namespace geo

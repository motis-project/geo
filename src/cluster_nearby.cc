#include "geo/cluster_nearby.h"

#include <cmath>
#include <cstddef>
#include <algorithm>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace geo {

constexpr float kEarthRadius_f = 6371000.;

namespace detail {

constexpr auto const kPi = static_cast<float>(M_PI);

struct latlng_f {
  latlng_f() = default;
  latlng_f(float lat, float lng) : lat_(lat), lng_(lng) {}

  float lat_, lng_;
};

struct bounding_box_f {
  float lat_max_, lat_min_, lng_max_, lng_min_;
};

inline float gc_distance_f(latlng_f const& a, latlng_f const& b) {
  auto const to_rad = [](float const deg) { return deg * kPi / 180.0F; };

  auto const u = std::sin((to_rad(b.lat_) - to_rad(a.lat_)) / 2);
  auto const v = std::sin((to_rad(b.lng_) - to_rad(a.lng_)) / 2);
  return 2.0F * kEarthRadius_f *
         std::asin(std::sqrt(u * u + std::cos(to_rad(a.lat_)) *
                                         std::cos(to_rad(b.lat_)) * v * v));
}

inline bounding_box_f compute_bounding_box(latlng_f const& center,
                                           float const dist) {
  // http://gis.stackexchange.com/a/2980
  float offset_lat = (dist / kEarthRadius_f) * 180.0F / kPi;
  float offset_lng =
      (dist / (kEarthRadius_f * std::cos(center.lat_ * kPi / 180.0F))) *
      180.0F / kPi;

  // clang-format off
  return {center.lat_ + offset_lat,
          center.lat_ - offset_lat,
          center.lng_ + offset_lng,
          center.lng_ - offset_lng};
  // clang-format on
}
}  // namespace detail

inline std::vector<cluster_id_t> make_single_linkage_clusters(
    std::vector<detail::latlng_f> const& coords, float const max_dist) {
  std::vector<cluster_id_t> clusters(coords.size(), NO_CLUSTER);
  for (unsigned i = 0; i < coords.size(); ++i) {
    std::vector<cluster_id_t> cluster_candidates;

    auto const& s1 = coords[i];
    auto box = detail::compute_bounding_box(s1, max_dist);

    for (unsigned j = 0; j < i; ++j) {
      auto const& s2 = coords[j];

      // not in bounding box
      // clang-format off
      if ((s2.lat_ < box.lat_min_) || (s2.lat_ > box.lat_max_) ||
          (s2.lng_ < box.lng_min_) || (s2.lng_ > box.lng_max_)) {
        continue;
      }
      // clang-format on

      // not in exact distance
      if (detail::gc_distance_f(s1, s2) > max_dist) {
        continue;
      }

      cluster_candidates.push_back(j);
    }

    // create new cluster with all candidates and their current cluster
    if (!cluster_candidates.empty()) {
      clusters[i] = i;
      for (auto candidate_id : cluster_candidates) {
        auto old_id = clusters[candidate_id];
        clusters[candidate_id] = i;

        if (old_id != NO_CLUSTER) {
          std::replace(begin(clusters), end(clusters), old_id, i);
        }
      }
    }
  }

  for (auto i = 0U; i < clusters.size(); ++i) {
    if (clusters[i] == NO_CLUSTER) {
      clusters[i] = i;
    }
  }
  return clusters;
}

inline std::vector<cluster_id_t> make_complete_linkage_clusters(
    std::vector<detail::latlng_f> const& coords, float const max_dist) {
  if (coords.empty()) {
    throw std::runtime_error{"make_complete_linkage_clusters: empty coords"};
  }

  std::vector<cluster_id_t> clusters(coords.size());
  std::generate(begin(clusters), end(clusters),
                [i = 0UL]() mutable { return i++; });

  std::vector<float> distances(clusters.size() * clusters.size(),
                               std::numeric_limits<float>::lowest());

  while (std::any_of(begin(clusters), end(clusters),
                     [&](auto const i) { return i != clusters.front(); })) {
    std::fill(begin(distances), end(distances),
              std::numeric_limits<float>::lowest());

    // compute cluster distance matrix as:
    // maximum distance between members
    // a.k.a Complete-Linkage on wikipedia
    for (auto i = 0U; i < coords.size(); ++i) {
      for (auto j = 0U; j < i; j++) {
        auto ci = clusters[i];
        auto cj = clusters[j];

        if (ci == cj) {
          continue;
        }
        float distance = gc_distance_f(coords[i], coords[j]);
        auto& max_dist = distances[ci * clusters.size() + cj];
        if (distance > max_dist) {
          max_dist = distance;
        }
      }
    }

    // find nearest clusters
    auto min_dist = std::numeric_limits<float>::max();
    auto best_i = 0U;
    auto best_j = 0U;
    for (auto i = 0U; i < coords.size(); ++i) {
      for (auto j = 0U; j < i; j++) {
        auto const& distance = distances[i * clusters.size() + j];
        if (distance >= 0 && distance <= max_dist) {
          min_dist = distance;
          best_i = i;
          best_j = j;
        }
      }
    }

    if (min_dist > max_dist) {
      break;
    }

    // merge clusters
    auto ci = clusters[best_i];
    auto cj = clusters[best_j];
    std::replace(begin(clusters), end(clusters), ci, cj);
  }

  return clusters;
}

std::vector<cluster_id_t> cluster_nearby(std::vector<latlng> const& coords_d,
                                         float const max_dist) {
  if (coords_d.empty()) {
    return {};
  }

  std::vector<detail::latlng_f> coords;
  coords.reserve(coords_d.size());
  for (auto const& ll : coords_d) {
    coords.emplace_back(static_cast<float>(ll.lat_),
                        static_cast<float>(ll.lng_));
  }

  // make single linkage clusters
  auto const sl_clusters = make_single_linkage_clusters(coords, max_dist);

  // prepare indices
  std::vector<std::pair<cluster_id_t, size_t>> sl_cluster_indices;
  sl_cluster_indices.reserve(sl_clusters.size());
  for (auto i = 0U; i < sl_clusters.size(); ++i) {
    sl_cluster_indices.emplace_back(sl_clusters[i], i);
  }
  std::sort(begin(sl_cluster_indices), end(sl_cluster_indices));

  // subdivide single linkage clusters to complete linkage clusters
  std::vector<cluster_id_t> clusters(coords.size());
  std::generate(begin(clusters), end(clusters),
                [i = 0UL]() mutable { return i++; });
  auto const make_clusters = [&](auto const lb, auto const ub) {
    if (std::distance(lb, ub) < 3) {
      for (auto it = lb; it != ub; ++it) {
        clusters[it->second] = static_cast<cluster_id_t>(lb->second);
      }
      return;  // triangle inequality impossible because no triangle ;)
    }

    std::vector<detail::latlng_f> cl_coords;
    cl_coords.reserve(std::distance(lb, ub));
    std::transform(lb, ub, std::back_inserter(cl_coords),
                   [&](auto const& pair) { return coords[pair.second]; });

    auto cl_clusters = make_complete_linkage_clusters(cl_coords, max_dist);

    for (auto i = 0U; i < cl_clusters.size(); ++i) {
      clusters[(lb + i)->second] = clusters[(lb + cl_clusters[i])->second];
    }
  };

  // use indices to iterate
  auto lower = begin(sl_cluster_indices);
  while (lower != end(sl_cluster_indices)) {
    auto upper = lower;
    while (upper != end(sl_cluster_indices) && lower->first == upper->first) {
      ++upper;
    }
    make_clusters(lower, upper);
    lower = upper;
  }

  return clusters;
}

}  // namespace geo

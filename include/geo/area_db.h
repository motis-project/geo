#pragma once

#include <filesystem>
#include <memory>
#include <ranges>

#include "cista/char_traits.h"
#include "cista/containers/mmap_vec.h"
#include "cista/containers/nvec.h"
#include "cista/mmap.h"

#include "rtree.h"

#include "tg.h"

#include "utl/concat.h"
#include "utl/enumerate.h"
#include "utl/erase_if.h"
#include "utl/parallel_for.h"
#include "utl/zip.h"

#include "geo/box.h"
#include "geo/fixed_latlng.h"
#include "geo/latlng.h"

namespace geo {

template <typename Idx>
struct area_db_lookup;

template <typename Idx>
struct area_db_storage {
  friend struct area_db_lookup<Idx>;

  template <typename T>
  using mm_vec = cista::basic_mmap_vec<T, std::uint64_t>;

  template <typename K, typename V, std::size_t N>
  using mm_nvec =
      cista::basic_nvec<K, mm_vec<V>, mm_vec<std::uint64_t>, N, std::uint64_t>;

  using inner_rings_t = mm_nvec<Idx, fixed_latlng, 3U>;
  using outer_rings_t = mm_nvec<Idx, fixed_latlng, 2U>;

  area_db_storage(std::filesystem::path const& p,
                  cista::mmap::protection const mode)
      : p_{std::move(p)},
        mode_{mode},
        outer_rings_{{mm_vec<std::uint64_t>{mm("outer_rings_idx_0.bin")},
                      mm_vec<std::uint64_t>{mm("outer_rings_idx_1.bin")}},
                     mm_vec<fixed_latlng>{mm("outer_rings_data.bin")}},
        inner_rings_{{mm_vec<std::uint64_t>{mm("inner_rings_idx_0.bin")},
                      mm_vec<std::uint64_t>{mm("inner_rings_idx_1.bin")},
                      mm_vec<std::uint64_t>{mm("inner_rings_idx_2.bin")}},
                     mm_vec<fixed_latlng>{mm("inner_rings_data.bin")}} {}

  template <typename OuterRings, typename InnerRings>
  void add_area(OuterRings&& outers, InnerRings&& inners) {
    outer_rings_.emplace_back(outers);
    inner_rings_.emplace_back(inners);
  }

  template <typename Area>
  void add_osmium_area(Area&& a) {
    namespace v = std::ranges::views;
    auto const nodes_to_coordinates = [](auto&& n) {
      return geo::fixed_latlng::from_latlng({n.lat(), n.lon()});
    };
    auto const ring_to_coordinates = [&](auto&& r) {
      return r | v::transform(nodes_to_coordinates);
    };
    auto const outers = [&]() {
      return a.outer_rings() | v::transform(ring_to_coordinates);
    };
    auto const inners = [&]() {
      return a.outer_rings() | v::transform([&](auto&& r) {
               return a.inner_rings(r) | v::transform(ring_to_coordinates);
             });
    };
    add_area(outers(), inners());
  }

private:
  cista::mmap mm(char const* file) {
    return cista::mmap{(p_ / file).generic_string().c_str(), mode_};
  }

  std::filesystem::path p_;
  cista::mmap::protection mode_;
  outer_rings_t outer_rings_;
  inner_rings_t inner_rings_;
};

template <typename Idx>
struct area_db_lookup {
  using rtree_results_t = std::basic_string<Idx, cista::char_traits<Idx>>;

  area_db_lookup(area_db_storage<Idx> const& s) : rtree_{rtree_new()} {
    struct tmp {
      std::vector<tg_point> ring_tmp_;
      std::vector<tg_ring*> inner_tmp_;
      std::vector<tg_poly*> polys_tmp_;
      rtree_results_t areas_;
    };

    auto const convert_ring = [](std::vector<tg_point>& ring_tmp,
                                 auto&& osm_ring) -> tg_ring* {
      ring_tmp.clear();
      for (auto const& p : osm_ring) {
        ring_tmp.emplace_back(tg_point{p.lng(), p.lat()});
      }
      return tg_ring_new(ring_tmp.data(), static_cast<int>(ring_tmp.size()));
    };

    auto mutex = std::mutex{};
    auto polys_to_free = std::vector<tg_poly*>{};

    idx_.resize(s.outer_rings_.size());
    utl::parallel_for_run_threadlocal<tmp>(
        s.outer_rings_.size(), [&](tmp& tmp, std::size_t const i) {
          tmp.polys_tmp_.clear();

          auto const area_idx = Idx{i};
          auto const& outer_rings = s.outer_rings_[area_idx];

          auto box = geo::box{};
          for (auto const [outer_idx, outer_ring] :
               utl::enumerate(outer_rings)) {
            tmp.inner_tmp_.clear();
            for (auto const inner_ring : s.inner_rings_[area_idx][outer_idx]) {
              tmp.inner_tmp_.emplace_back(
                  convert_ring(tmp.ring_tmp_, inner_ring));
            }

            for (auto const& c : outer_ring) {
              box.extend(c);
            }

            auto const outer = convert_ring(tmp.ring_tmp_, outer_ring);
            auto const poly =
                tg_poly_new(outer, tmp.inner_tmp_.data(),
                            static_cast<int>(tmp.inner_tmp_.size()));
            tg_ring_free(outer);
            tmp.polys_tmp_.emplace_back(poly);
          }

          auto const min_corner = box.min_.lnglat();
          auto const max_corner = box.max_.lnglat();

          idx_[i] = tg_geom_new_multipolygon(
              tmp.polys_tmp_.data(), static_cast<int>(tmp.polys_tmp_.size()));

          {
            auto const lock = std::scoped_lock{mutex};
            utl::concat(polys_to_free, tmp.polys_tmp_);
            rtree_insert(rtree_, min_corner.data(), max_corner.data(),
                         reinterpret_cast<void*>(
                             static_cast<std::uintptr_t>(to_idx(area_idx))));
          }

          for (auto const& x : tmp.inner_tmp_) {
            tg_ring_free(x);
          }
        });

    for (auto* x : polys_to_free) {
      tg_poly_free(x);
    }
  }

  area_db_lookup(area_db_lookup&& o) { *this = std::move(o); }

  area_db_lookup& operator=(area_db_lookup&& o) {
    if (&o != this) {
      rtree_ = o.rtree_;
      idx_ = std::move(o.idx_);
      o.rtree_ = nullptr;
    }
    return *this;
  }

  area_db_lookup(area_db_lookup const&) = delete;
  area_db_lookup& operator=(area_db_lookup const&) = delete;

  ~area_db_lookup() {
    for (auto const& mp : idx_) {
      tg_geom_free(mp);
    }
    if (rtree_ != nullptr) {
      rtree_free(rtree_);
    }
  }

  void lookup(geo::latlng const& c, rtree_results_t& rtree_results) const {
    auto const min = c.lnglat();
    rtree_results.clear();
    rtree_search(
        rtree_, min.data(), nullptr,
        [](double const*, double const*, void const* item, void* udata) {
          auto const area = Idx{static_cast<typename Idx::value_t>(
              reinterpret_cast<std::intptr_t>(item))};
          reinterpret_cast<rtree_results_t*>(udata)->push_back(area);
          return true;
        },
        &rtree_results);
    utl::erase_if(rtree_results, [&](Idx const a) { return !is_within(c, a); });
  }

  bool is_within(geo::latlng const c, Idx const area) const {
    auto const point = tg_geom_new_point(tg_point{c.lng(), c.lat()});
    auto const result = tg_geom_within(point, idx_[to_idx(area)]);
    tg_geom_free(point);
    return result;
  }

  rtree* rtree_;
  std::vector<tg_geom*> idx_;
};

}  // namespace geo
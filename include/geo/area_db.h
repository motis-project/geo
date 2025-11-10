#pragma once

#include <filesystem>

#include "cista/containers/mmap_vec.h"
#include "cista/containers/nvec.h"
#include "cista/mmap.h"

namespace geo {

struct area_db {
  using area_idx_t = cista::strong<std::uint32_t, struct area_idx_>;

  template <typename T>
  using mm_vec = cista::basic_mmap_vec<T, std::uint64_t>;

  template <typename K, typename V, std::size_t N>
  using mm_nvec =
      cista::basic_nvec<K, mm_vec<V>, mm_vec<std::uint64_t>, N, std::uint64_t>;

  struct persistence {
    struct coordinates {
      std::uint32_t lat_, lng_;
    };
    using inner_rings_t = mm_nvec<area_idx_t, coordinates, 3U>;
    using outer_rings_t = mm_nvec<area_idx_t, coordinates, 2U>;
    persistence(std::filesystem::path, cista::mmap::protection);
    ~persistence();
  };
};

}  // namespace geo
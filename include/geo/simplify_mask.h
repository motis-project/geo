#pragma once

#include <cstdint>
#include <cstring>

#include <string>
#include <vector>

#include <cmath>
#include "geo/constants.h"
#include "geo/latlng.h"
#include "geo/polyline.h"
#include "geo/webmercator.h"

namespace geo {

constexpr auto kMaxSimplifyZoomLevel = 20;
constexpr auto kSimplifyZoomLevels = kMaxSimplifyZoomLevel + 1;
using simplify_mask_t = std::vector<std::vector<bool>>;

namespace detail {

template <typename Coord>
uint64_t sq_perpendicular_dist(Coord const& source, Coord const& target,
                               Coord const& test) {
  Coord const slope_vec{target.y() - source.y(), target.x() - source.x()};
  Coord const rel_coord{test.y() - source.y(), test.x() - source.x()};

  // dot product of two un-normalized vectors
  auto const unnormed_ratio = static_cast<double>(
      slope_vec.x() * rel_coord.x() + slope_vec.y() * rel_coord.y());
  auto const sq_length = static_cast<double>(slope_vec.x() * slope_vec.x() +
                                             slope_vec.y() * slope_vec.y());

  double proj_x = NAN, proj_y = NAN;
  if (sq_length < std::numeric_limits<double>::epsilon()) {
    proj_x = static_cast<double>(source.x());
    proj_y = static_cast<double>(source.y());
  } else {
    double const normed_ratio = unnormed_ratio / sq_length;
    double const clamped_ratio = std::max(std::min(normed_ratio, 1.), 0.);

    proj_x = (1. - clamped_ratio) * static_cast<double>(source.x()) +
             static_cast<double>(target.x()) * clamped_ratio;
    proj_y = (1. - clamped_ratio) * static_cast<double>(source.y()) +
             static_cast<double>(target.y()) * clamped_ratio;
  }

  auto const dx = proj_x - static_cast<double>(test.x());
  auto const dy = proj_y - static_cast<double>(test.y());
  return dx * dx + dy * dy;
}

using range_t = std::pair<size_t, size_t>;

// Reusable scratch for the Douglas-Peucker level sweep. Hoist one out of the
// per-geometry loop (e.g. `thread_local`) so the buffers keep their capacity
// across calls instead of re-allocating for every geometry. `stack_` is a plain
// vector used as a LIFO -- unlike `std::stack{vec}`, a reserve on it actually
// sticks.
struct simplify_ctx {
  std::vector<range_t> stack_;
  std::vector<bool> mask_;
  std::vector<uint8_t> first_level_;
};

template <typename Polyline>
bool process_level(Polyline const& line, uint64_t const threshold,
                   simplify_ctx& ctx, uint8_t const level = 0,
                   bool const record_first_level = false) {
  auto& stack = ctx.stack_;
  auto& mask = ctx.mask_;
  assert(stack.empty());

  auto last = 0U;
  for (auto i = 1U; i < mask.size(); ++i) {
    if (mask[i]) {
      if (i - last > 1U) {
        stack.emplace_back(last, i);
      }

      last = i;
    }
  }

  if (stack.empty()) {
    return true;
  }

  while (!stack.empty()) {
    auto const pair = stack.back();
    stack.pop_back();

    uint64_t max_dist = 0;
    auto farthest_entry_index = pair.second;

    for (auto idx = pair.first + 1; idx != pair.second; ++idx) {
      auto const dist =
          sq_perpendicular_dist(line[pair.first], line[pair.second], line[idx]);

      if (dist > max_dist && dist >= threshold) {
        farthest_entry_index = idx;
        max_dist = dist;
      }
    }

    if (max_dist >= threshold) {
      mask[farthest_entry_index] = true;
      if (record_first_level) {
        ctx.first_level_[farthest_entry_index] = level;
      }
      if (pair.first < farthest_entry_index) {
        stack.emplace_back(pair.first, farthest_entry_index);
      }
      if (farthest_entry_index < pair.second) {
        stack.emplace_back(farthest_entry_index, pair.second);
      }
    }
  }

  return false;
}

}  // namespace detail

template <typename Polyline>
simplify_mask_t make_simplify_mask(Polyline const& line,
                                   uint32_t const pixel_precision = 1) {
  simplify_mask_t result;

  detail::simplify_ctx ctx;
  ctx.mask_.assign(line.size(), false);
  ctx.mask_.front() = true;
  ctx.mask_.back() = true;
  ctx.stack_.reserve(line.size());

  for (auto z = 0; z <= kMaxSimplifyZoomLevel; ++z) {
    uint64_t const delta = static_cast<uint64_t>(pixel_precision)
                           << (kMaxSimplifyZoomLevel - z);
    uint64_t const threshold = delta * delta;

    auto const done = detail::process_level(line, threshold, ctx);

    if (done) {
      for (auto i = z; i <= kMaxSimplifyZoomLevel; ++i) {
        result.push_back(ctx.mask_);
      }
      break;
    }

    result.push_back(ctx.mask_);
  }

  assert(result.size() == kMaxSimplifyZoomLevel + 1);
  return result;
}

template <>
inline simplify_mask_t make_simplify_mask<geo::polyline>(
    geo::polyline const& input, uint32_t const pixel_precision) {
  using proj = webmercator<4096, kMaxSimplifyZoomLevel>;

  std::vector<pixel_xy> line;
  line.reserve(input.size());
  std::transform(
      begin(input), end(input), std::back_inserter(line), [](auto const& in) {
        return proj::merc_to_pixel(latlng_to_merc(in), kMaxSimplifyZoomLevel);
      });
  return make_simplify_mask(line, pixel_precision);
}

template <typename Polyline>
void apply_simplify_mask(std::vector<bool> const& mask, Polyline& line) {
  assert(mask.size() == line.size());
  if (mask.empty()) {
    return;
  }
  assert(mask.at(0) == true);

  auto first = std::next(begin(line));
  size_t pos = 1;
  for (auto it = first; it != end(line); ++pos, ++it) {
    if (mask[pos]) {
      *first++ = std::move(*it);
    }
  }

  line.erase(first, end(line));
}

// Pack the cumulative per-level simplify bitmask into the serialized byte
// stream: a 4-byte bitset of which levels are stored, a 4-byte point count,
// then, for each stored level, one bit per point (LSB-first, bits run
// continuously across levels). `num_levels` levels are considered; level `i` is
// emitted iff `level_stored(i)` and its bit for point `pt` is `bit(i, pt)`.
// Both a materialized `simplify_mask_t` and the fused `first_level`
// representation feed this via different callbacks.
template <typename LevelStored, typename Bit>
inline std::string emit_simplify_mask(uint32_t const n, int const num_levels,
                                      LevelStored&& level_stored, Bit&& bit) {
  uint32_t lvls = 0;
  std::string out(2 * sizeof(uint32_t), '\0');
  std::memcpy(out.data() + sizeof(uint32_t), &n, sizeof(uint32_t));

  char buf = 0;
  auto buf_pos = 0;
  for (auto i = 0; i < num_levels; ++i) {
    if (!level_stored(i)) {
      continue;
    }
    lvls |= 1U << i;
    for (uint32_t pt = 0; pt < n; ++pt) {
      buf |= static_cast<int>(bit(i, pt)) << buf_pos;
      if (++buf_pos == 8) {
        out.push_back(buf);
        buf = 0;
        buf_pos = 0;
      }
    }
  }
  if (buf_pos != 0) {
    out.push_back(buf);
  }

  std::memcpy(out.data(), &lvls, sizeof(uint32_t));
  return out;
}

inline std::string serialize_simplify_mask(simplify_mask_t const& mask) {
  auto const num_levels = static_cast<int>(mask.size());
  return emit_simplify_mask(
      static_cast<uint32_t>(mask[0].size()), num_levels,
      // A level is stored iff it is the last or differs from the next.
      [&](int const i) {
        return i + 1 >= num_levels || mask[i] != mask[i + 1];
      },
      [&](int const i, uint32_t const pt) { return mask[i][pt]; });
}

// Fused equivalent of `serialize_simplify_mask(make_simplify_mask(line))` that
// avoids materializing (and heap-allocating) one `std::vector<bool>` snapshot
// per zoom level. Instead it records, per point, the first zoom level at which
// it is kept (`first_level`), then emits the exact same byte stream. The masks
// are cumulative (a point kept at level z stays kept), so `mask[i][pt]` is
// simply `first_level[pt] <= i`.
template <typename Polyline>
std::string make_serialize_simplify_mask(Polyline const& line,
                                         uint32_t const pixel_precision = 1) {
  auto const n = static_cast<uint32_t>(line.size());

  // Reused across calls on the same thread to avoid re-allocating the scratch
  // buffers for every geometry in the (hot) feature-packing loop. `assign`
  // keeps the existing capacity. kSimplifyZoomLevels marks "never kept".
  static thread_local detail::simplify_ctx ctx;
  auto& first_level = ctx.first_level_;
  first_level.assign(n, static_cast<uint8_t>(kSimplifyZoomLevels));
  ctx.mask_.assign(n, false);
  if (n > 0) {
    first_level.front() = 0;
    first_level.back() = 0;
    ctx.mask_.front() = true;
    ctx.mask_.back() = true;
  }
  ctx.stack_.clear();  // keeps capacity for the next geometry

  bool done = false;
  for (auto z = 0; z <= kMaxSimplifyZoomLevel && !done; ++z) {
    uint64_t const delta = static_cast<uint64_t>(pixel_precision)
                           << (kMaxSimplifyZoomLevel - z);
    uint64_t const threshold = delta * delta;
    done = detail::process_level(line, threshold, ctx, static_cast<uint8_t>(z),
                                 /*record_first_level=*/true);
  }

  // added[z] == "level z introduced at least one new point".
  bool added[kSimplifyZoomLevels] = {};
  for (auto const lvl : first_level) {
    if (lvl < kSimplifyZoomLevels) {
      added[lvl] = true;
    }
  }

  // Same byte stream as serialize_simplify_mask, sourced from `first_level`:
  // level i is stored iff it is the last or level i+1 added points, and a point
  // is set at level i iff it was first kept at or before i.
  return emit_simplify_mask(
      n, kSimplifyZoomLevels,
      [&](int const i) {
        return i + 1 >= kSimplifyZoomLevels || added[i + 1];
      },
      [&](int const i, uint32_t const pt) { return first_level[pt] <= i; });
}

struct simplify_mask_reader {
  explicit simplify_mask_reader(char const* data, uint32_t req_lvl) {
    assert(req_lvl <= kMaxSimplifyZoomLevel);

    uint32_t lvls = 0;
    std::memcpy(&lvls, data, sizeof(uint32_t));
    assert(lvls != 0);

    std::memcpy(&size_, data + sizeof(uint32_t), sizeof(uint32_t));

    uint32_t skipped_levels = 0U;
    for (auto i = 0U; i < 32U; ++i) {
      if (i >= req_lvl) {
        break;
      }
      if ((lvls & (1U << i)) != 0) {
        ++skipped_levels;
      }
    }
    assert(lvls >= (1U << skipped_levels));

    base_ptr_ = data + 2 * sizeof(uint32_t);
    offset_ = skipped_levels * size_;

    assert(get_bit(0) == true);
  }

  bool get_bit(size_t const pos) const {
    auto byte = *(base_ptr_ + (offset_ + pos) / 8);
    return ((byte >> (offset_ + pos) % 8) & 0x1) != 0;
  }

  uint32_t size_{};
  char const* base_ptr_;
  uint32_t offset_;
};

template <typename Polyline>
void apply_simplify_mask(std::string const& mask, int req_lvl, Polyline& line) {
  simplify_mask_reader reader{mask.data(), static_cast<uint32_t>(req_lvl)};

  auto first = std::next(begin(line));
  size_t pos = 1;
  for (auto it = first; it != end(line); ++pos, ++it) {
    if (reader.get_bit(pos)) {
      *first++ = std::move(*it);
    }
  }

  line.erase(first, end(line));
}

template <typename Polyline>
void simplify(Polyline& line, uint64_t const pixel_precision = 1) {
  if (line.empty()) {
    throw std::runtime_error{"simplify: empty polyline"};
  }
  detail::simplify_ctx ctx;
  ctx.mask_.assign(line.size(), false);
  ctx.mask_.front() = true;
  ctx.mask_.back() = true;
  ctx.stack_.reserve(line.size());

  uint64_t const threshold = pixel_precision * pixel_precision;

  detail::process_level(line, threshold, ctx);

  apply_simplify_mask(ctx.mask_, line);
}

}  // namespace geo

#pragma once

#include <cstdint>
#include <cstring>

#include <sstream>
#include <stack>
#include <string>
#include <vector>

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

  double proj_x, proj_y;
  if (sq_length < std::numeric_limits<double>::epsilon()) {
    proj_x = static_cast<double>(source.x());
    proj_y = static_cast<double>(source.y());
  } else {
    double const normed_ratio = unnormed_ratio / sq_length;
    double const clamped_ratio = std::max(std::min(normed_ratio, 1.), 0.);

    proj_x = (1. - clamped_ratio) * source.x() + target.x() * clamped_ratio;
    proj_y = (1. - clamped_ratio) * source.y() + target.y() * clamped_ratio;
  }

  auto const dx = proj_x - test.x();
  auto const dy = proj_y - test.y();
  return dx * dx + dy * dy;
}

using range_t = std::pair<size_t, size_t>;
using stack_t = std::stack<range_t, std::vector<range_t>>;

template <typename Polyline>
bool process_level(Polyline const& line, uint64_t const threshold,
                   stack_t& stack, std::vector<bool>& mask) {
  assert(stack.empty());

  auto last = 0;
  for (auto i = 1u; i < mask.size(); ++i) {
    if (mask[i]) {
      if (i - last > 1) {
        stack.emplace(last, i);
      }

      last = i;
    }
  }

  if (stack.empty()) {
    return true;
  }

  while (!stack.empty()) {
    auto const pair = stack.top();
    stack.pop();

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
      if (pair.first < farthest_entry_index) {
        stack.emplace(pair.first, farthest_entry_index);
      }
      if (farthest_entry_index < pair.second) {
        stack.emplace(farthest_entry_index, pair.second);
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

  std::vector<bool> mask(line.size(), false);
  mask.front() = true;
  mask.back() = true;

  std::vector<detail::range_t> stack_mem;
  stack_mem.reserve(line.size());
  detail::stack_t stack{stack_mem};

  for (auto z = 0; z <= kMaxSimplifyZoomLevel; ++z) {
    uint64_t const delta = static_cast<uint64_t>(pixel_precision)
                           << (kMaxSimplifyZoomLevel - z);
    uint64_t const threshold = delta * delta;

    auto const done = detail::process_level(line, threshold, stack, mask);

    if (done) {
      for (auto i = z; i <= kMaxSimplifyZoomLevel; ++i) {
        result.push_back(mask);
      }
      break;
    }

    result.push_back(mask);
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

inline std::string serialize_simplify_mask(simplify_mask_t const& mask) {
  auto lvls = uint32_t{0};
  auto size = static_cast<uint32_t>(mask[0].size());

  std::stringstream ss;
  ss.write(reinterpret_cast<char*>(&lvls), sizeof lvls);
  ss.write(reinterpret_cast<char*>(&size), sizeof size);

  char buf = 0;
  auto buf_pos = 0;

  for (auto i = 0u; i < mask.size(); ++i) {
    if (i + 1 < mask.size() && mask[i] == mask[i + 1]) {
      continue;
    }

    lvls |= 1 << i;

    for (auto bit : mask[i]) {
      buf |= bit << buf_pos;

      if (++buf_pos == 8) {
        ss.put(buf);
        buf = 0;
        buf_pos = 0;
      }
    }
  }

  if (buf_pos != 0) {
    ss.put(buf);
  }

  auto str = ss.str();
  std::memcpy(const_cast<char*>(str.data()),
              reinterpret_cast<char const*>(&lvls), sizeof lvls);
  return str;
}

struct simplify_mask_reader {
  explicit simplify_mask_reader(char const* data, uint32_t req_lvl) {
    assert(req_lvl <= kMaxSimplifyZoomLevel);

    uint32_t lvls;
    std::memcpy(&lvls, data, sizeof(uint32_t));
    assert(lvls != 0);

    std::memcpy(&size_, data + sizeof(uint32_t), sizeof(uint32_t));

    uint32_t skipped_levels = 0u;
    for (auto i = 0u; i < 32u; ++i) {
      if (i >= req_lvl) {
        break;
      }
      if ((lvls & (1u << i)) != 0) {
        ++skipped_levels;
      }
    }
    assert(lvls >= (1u << skipped_levels));

    base_ptr_ = data + 2 * sizeof(uint32_t);
    offset_ = skipped_levels * size_;

    assert(get_bit(0) == true);
  }

  bool get_bit(size_t const pos) const {
    auto byte = *(base_ptr_ + (offset_ + pos) / 8);
    return (byte >> (offset_ + pos) % 8) & 0x1;
  }

  uint32_t size_;
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

}  // namespace geo

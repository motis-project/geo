#include "geo/simplify_mask.h"

#include <sstream>
#include <stack>

#include "geo/constants.h"
#include "geo/webmercator.h"

namespace geo {

using proj = webmercator<4096, kMaxSimplifyZoomLevel>;

uint64_t sq_perpendicular_dist(pixel_xy const& source, pixel_xy const& target,
                               pixel_xy const& test) {
  pixel_xy const slope_vec{target.y_ - source.y_, target.x_ - source.x_};
  pixel_xy const rel_coord{test.y_ - source.y_, test.x_ - source.x_};

  // dot product of two un-normalized vectors
  double const unnormed_ratio =
      slope_vec.x_ * rel_coord.x_ + slope_vec.y_ * rel_coord.y_;
  double const sq_length =
      slope_vec.x_ * slope_vec.x_ + slope_vec.y_ * slope_vec.y_;

  double proj_x, proj_y;

  if (sq_length < std::numeric_limits<double>::epsilon()) {
    proj_x = source.x_;
    proj_y = source.y_;
  } else {
    double const normed_ratio = unnormed_ratio / sq_length;
    double const clamped_ratio = std::max(std::min(normed_ratio, 1.), 0.);

    proj_x = (1. - clamped_ratio) * source.x_ + target.x_ * clamped_ratio;
    proj_y = (1. - clamped_ratio) * source.y_ + target.y_ * clamped_ratio;
  }

  auto const dx = proj_x - test.x_;
  auto const dy = proj_y - test.y_;
  return dx * dx + dy * dy;
}

using range_t = std::pair<size_t, size_t>;
using stack_t = std::stack<range_t, std::vector<range_t>>;

bool process_level(std::vector<pixel_xy> const& line, uint64_t const threshold,
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

simplify_mask_t make_simplify_mask(polyline const& input,
                                   uint32_t const pixel_precision) {
  if (input.size() < 2) {
    return simplify_mask_t{kMaxSimplifyZoomLevel + 1,
                           std::vector<bool>(input.size(), true)};
  }

  std::vector<pixel_xy> line;
  line.reserve(input.size());
  std::transform(
      begin(input), end(input), std::back_inserter(line), [](auto const& in) {
        return proj::merc_to_pixel(latlng_to_merc(in), kMaxSimplifyZoomLevel);
      });

  simplify_mask_t result;

  std::vector<bool> mask(line.size(), false);
  mask.front() = true;
  mask.back() = true;

  std::vector<range_t> stack_mem;
  stack_mem.reserve(line.size());
  stack_t stack{stack_mem};

  for (auto z = 0; z <= kMaxSimplifyZoomLevel; ++z) {
    uint64_t const delta = static_cast<uint64_t>(pixel_precision)
                           << (kMaxSimplifyZoomLevel - z);
    uint64_t const threshold = delta * delta;

    auto const done = process_level(line, threshold, stack, mask);

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

std::string serialize_simplify_mask(simplify_mask_t const& mask) {
  uint32_t lvls = 0;
  uint32_t size = mask[0].size();

  std::stringstream ss;
  ss.write(reinterpret_cast<char*>(&lvls), sizeof lvls);
  ss.write(reinterpret_cast<char*>(&size), sizeof size);

  char buf = 0;
  auto buf_pos = 0;

  for(auto i = 0u; i < mask.size(); ++i) {
    if(i+1 < mask.size() && mask[i] == mask[i+1]){
      continue;
    }

    lvls |= 1 << i;

    for(auto bit : mask[i]) {
      buf |= bit << buf_pos;

      if(++buf_pos == 8) {
        ss.put(buf);
        buf = 0;
        buf_pos = 0;
      }
    }
  }

  if(buf_pos != 0) {
    ss.put(buf);
  }

  auto str = ss.str();
  *reinterpret_cast<uint32_t*>(const_cast<char*>(str.data())) = lvls;  // NOLINT
  return str;
}

}  // namespace geo

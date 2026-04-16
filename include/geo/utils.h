#pragma once

#include <iterator>

namespace geo {

template <typename Container, typename Fn>
void transform_erase(Container& c, Fn&& fn) {
  if (c.empty()) {
    return;
  }

  auto it = std::begin(c);
  fn(*it);

  for (auto it2 = std::next(it); it2 != std::end(c); ++it2) {
    fn(*it2);
    if (!(*it == *it2)) {
      *++it = std::move(*it2);
    }
  }

  c.erase(++it, std::end(c));
}

}  // namespace geo

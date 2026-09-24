#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

#include "core/types.h"

namespace fell {

using usize = std::size_t;
using isize = std::ptrdiff_t;

using uptr = std::uintptr_t;
using sptr = std::intptr_t;

template <typename T>
inline constexpr T kMinValue{std::numeric_limits<T>::lowest()};

template <typename T>
inline constexpr T kMaxValue{std::numeric_limits<T>::max()};

}  // namespace fell

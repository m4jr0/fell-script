#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>

namespace fell {

using s8 = std::int8_t;
using s16 = std::int16_t;
using s32 = std::int32_t;
using s64 = std::int64_t;
using u8 = std::uint8_t;
using u16 = std::uint16_t;
using u32 = std::uint32_t;
using u64 = std::uint64_t;
using f32 = float;
using f64 = double;
using usize = std::size_t;
using isize = std::ptrdiff_t;
using uptr = std::uintptr_t;
using sptr = std::intptr_t;

template <typename T>
inline constexpr T kMinValue{std::numeric_limits<T>::lowest()};

template <typename T>
inline constexpr T kMaxValue{std::numeric_limits<T>::max()};

}  // namespace fell

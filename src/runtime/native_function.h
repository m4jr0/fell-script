#pragma once

#include "core/type.h"

namespace fell {

using NativeFunctionId = u32;

inline constexpr NativeFunctionId kPrintNativeFunctionId{0};
inline constexpr NativeFunctionId kAssertNativeFunctionId{1};

inline constexpr NativeFunctionId kInvalidNativeFunctionId{
    static_cast<NativeFunctionId>(-1)};

}  // namespace fell

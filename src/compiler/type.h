#pragma once

#include "core/types.h"

namespace fell {

enum class Type {
  kInvalid,
  kError,

  kS8,
  kS16,
  kS32,
  kS64,

  kU8,
  kU16,
  kU32,
  kU64,

  kF32,
  kF64,
};

bool IsNumericType(Type type);
bool CanImplicitlyConvert(Type from, Type to);
Type FindCommonNumericType(Type left, Type right);
bool CanRepresentInteger(Type type, u64 value);

}  // namespace fell

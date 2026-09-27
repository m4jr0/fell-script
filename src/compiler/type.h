#pragma once

#include "core/string.h"
#include "core/types.h"

namespace fell {

enum class Type {
  kInvalid,
  kError,

  kBool,

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
bool IsSignedInteger(Type type);
bool IsUnsignedInteger(Type type);
bool IsFloatingPoint(Type type);

bool CanImplicitlyConvert(Type from, Type to);
bool CanRepresentInteger(Type type, u64 value);
bool CanRepresentNegativeInteger(Type type, u64 magnitude);

Type FindCommonNumericType(Type left, Type right);

StringView ToString(Type type);

}  // namespace fell

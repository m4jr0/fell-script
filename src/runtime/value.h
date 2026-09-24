#pragma once

#include "core/string.h"
#include "core/types.h"

namespace fell {

enum class ValueType : u8 {
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

union ValueData {
  s64 s64_value;
  u64 u64_value;
  f64 f64_value;
};

struct Value {
  ValueType type;
  ValueData data;
};

String ToString(const Value& value);

}  // namespace fell

#pragma once

#include "core/string.h"
#include "core/type.h"

namespace fell {

enum class ValueType : u8 {
  kInvalid,

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

  kString,
};

struct RuntimeString {
  String value;
};

union ValueData {
  bool bool_value{false};
  s64 s64_value;
  u64 u64_value;
  f64 f64_value;
  RuntimeString* string_value;
};

struct Value {
  ValueType type{ValueType::kInvalid};
  ValueData data{};
};

String ToString(const Value& value);

}  // namespace fell

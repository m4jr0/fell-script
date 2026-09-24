#include "runtime/value.h"

#include <charconv>

#include "core/assert.h"

namespace fell {
namespace {

template <typename T>
String NumberToString(T value) {
  char buffer[64]{};

  const auto [ptr, error]{
      std::to_chars(std::begin(buffer), std::end(buffer), value),
  };

  FELL_ASSERT(error == std::errc{});
  return String{buffer, ptr};
}

}  // namespace

String ToString(const Value& value) {
  switch (value.type) {
    case ValueType::kS8:
    case ValueType::kS16:
    case ValueType::kS32:
    case ValueType::kS64:
      return NumberToString(value.data.s64_value);

    case ValueType::kU8:
    case ValueType::kU16:
    case ValueType::kU32:
    case ValueType::kU64:
      return NumberToString(value.data.u64_value);

    case ValueType::kF32:
      return NumberToString(static_cast<f32>(value.data.f64_value));

    case ValueType::kF64:
      return NumberToString(value.data.f64_value);
  }

  FELL_UNREACHABLE();
}

}  // namespace fell
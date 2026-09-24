#include "compiler/type.h"

#include "core/assert.h"
#include "core/core.h"
#include "core/types.h"

namespace fell {

bool IsNumericType(Type type) {
  switch (type) {
    case Type::kS8:
    case Type::kS16:
    case Type::kS32:
    case Type::kS64:
    case Type::kU8:
    case Type::kU16:
    case Type::kU32:
    case Type::kU64:
    case Type::kF32:
    case Type::kF64:
      return true;

    case Type::kInvalid:
    case Type::kError:
      return false;
  }

  FELL_UNREACHABLE();
}

bool CanImplicitlyConvert(Type from, Type to) {
  FELL_ASSERT(IsNumericType(from));
  FELL_ASSERT(IsNumericType(to));

  if (from == to) {
    return true;
  }

  switch (from) {
    case Type::kS8:
      return to == Type::kS16 || to == Type::kS32 || to == Type::kS64 ||
             to == Type::kF32 || to == Type::kF64;

    case Type::kS16:
      return to == Type::kS32 || to == Type::kS64 || to == Type::kF32 ||
             to == Type::kF64;

    case Type::kS32:
      return to == Type::kS64 || to == Type::kF64;

    case Type::kS64:
      return false;

    case Type::kU8:
      return to == Type::kS16 || to == Type::kS32 || to == Type::kS64 ||
             to == Type::kU16 || to == Type::kU32 || to == Type::kU64 ||
             to == Type::kF32 || to == Type::kF64;

    case Type::kU16:
      return to == Type::kS32 || to == Type::kS64 || to == Type::kU32 ||
             to == Type::kU64 || to == Type::kF32 || to == Type::kF64;

    case Type::kU32:
      return to == Type::kS64 || to == Type::kU64 || to == Type::kF64;

    case Type::kU64:
      return false;

    case Type::kF32:
      return to == Type::kF64;

    case Type::kF64:
      return false;

    case Type::kInvalid:
    case Type::kError:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

Type FindCommonNumericType(Type left, Type right) {
  FELL_ASSERT(IsNumericType(left));
  FELL_ASSERT(IsNumericType(right));

  if (left == right) {
    return left;
  }

  constexpr Type kCandidates[]{
      Type::kS8,  Type::kU8,  Type::kS16, Type::kU16, Type::kS32,
      Type::kU32, Type::kS64, Type::kU64, Type::kF32, Type::kF64,
  };

  for (const Type candidate : kCandidates) {
    if (CanImplicitlyConvert(left, candidate) &&
        CanImplicitlyConvert(right, candidate)) {
      return candidate;
    }
  }

  return Type::kInvalid;
}

bool CanRepresentInteger(Type type, u64 value) {
  switch (type) {
    case Type::kS8:
      return value <= static_cast<u64>(kMaxValue<s8>);

    case Type::kS16:
      return value <= static_cast<u64>(kMaxValue<s16>);

    case Type::kS32:
      return value <= static_cast<u64>(kMaxValue<s32>);

    case Type::kS64:
      return value <= static_cast<u64>(kMaxValue<s64>);

    case Type::kU8:
      return value <= kMaxValue<u8>;

    case Type::kU16:
      return value <= kMaxValue<u16>;

    case Type::kU32:
      return value <= kMaxValue<u32>;

    case Type::kU64:
      return true;

    case Type::kInvalid:
    case Type::kError:
    case Type::kF32:
    case Type::kF64:
      return false;
  }

  FELL_UNREACHABLE();
}

}  // namespace fell

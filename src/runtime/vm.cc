#include "runtime/vm.h"

#include <type_traits>

#include "core/assert.h"

namespace fell {
namespace {

template <typename Source, typename Destination>
ValueData ConvertSigned(ValueData source) {
  const Source value{static_cast<Source>(source.s64_value)};
  ValueData result{};

  if constexpr (std::is_floating_point_v<Destination>) {
    result.f64_value = static_cast<f64>(static_cast<Destination>(value));
  } else if constexpr (std::is_signed_v<Destination>) {
    result.s64_value = static_cast<s64>(static_cast<Destination>(value));
  } else {
    result.u64_value = static_cast<u64>(static_cast<Destination>(value));
  }

  return result;
}

template <typename Source, typename Destination>
ValueData ConvertUnsigned(ValueData source) {
  const Source value{static_cast<Source>(source.u64_value)};
  ValueData result{};

  if constexpr (std::is_floating_point_v<Destination>) {
    result.f64_value = static_cast<f64>(static_cast<Destination>(value));
  } else if constexpr (std::is_signed_v<Destination>) {
    result.s64_value = static_cast<s64>(static_cast<Destination>(value));
  } else {
    result.u64_value = static_cast<u64>(static_cast<Destination>(value));
  }

  return result;
}

template <typename Source, typename Destination>
ValueData ConvertFloat(ValueData source) {
  const Source value{static_cast<Source>(source.f64_value)};

  ValueData result{};
  result.f64_value = static_cast<f64>(static_cast<Destination>(value));

  return result;
}

template <typename T>
ValueData NegateSigned(ValueData operand) {
  ValueData result{};

  // TODO(m4jr0): Define signed integer overflow semantics.
  result.s64_value = static_cast<T>(-static_cast<T>(operand.s64_value));

  return result;
}

template <typename T>
ValueData NegateFloat(ValueData operand) {
  ValueData result{};
  result.f64_value = static_cast<f64>(-static_cast<T>(operand.f64_value));
  return result;
}

template <typename T>
ValueData MultiplySigned(ValueData left, ValueData right) {
  ValueData result{};
  // TODO(m4jr0): Define signed integer overflow semantics and avoid relying on
  // C++ signed overflow behavior.
  result.s64_value = static_cast<T>(static_cast<T>(left.s64_value) *
                                    static_cast<T>(right.s64_value));
  return result;
}

template <typename T>
ValueData MultiplyUnsigned(ValueData left, ValueData right) {
  ValueData result{};
  result.u64_value = static_cast<T>(static_cast<T>(left.u64_value) *
                                    static_cast<T>(right.u64_value));
  return result;
}

template <typename T>
ValueData MultiplyFloat(ValueData left, ValueData right) {
  ValueData result{};
  result.f64_value = static_cast<f64>(static_cast<T>(left.f64_value) *
                                      static_cast<T>(right.f64_value));
  return result;
}

template <typename T>
ValueData DivideSigned(ValueData left, ValueData right) {
  ValueData result{};

  // TODO(m4jr0): Define integer division-by-zero and signed overflow
  // semantics.
  result.s64_value = static_cast<T>(static_cast<T>(left.s64_value) /
                                    static_cast<T>(right.s64_value));
  return result;
}

template <typename T>
ValueData DivideUnsigned(ValueData left, ValueData right) {
  ValueData result{};

  // TODO(m4jr0): Define integer division-by-zero semantics.
  result.u64_value = static_cast<T>(static_cast<T>(left.u64_value) /
                                    static_cast<T>(right.u64_value));
  return result;
}

template <typename T>
ValueData DivideFloat(ValueData left, ValueData right) {
  ValueData result{};
  result.f64_value = static_cast<f64>(static_cast<T>(left.f64_value) /
                                      static_cast<T>(right.f64_value));
  return result;
}

template <typename T>
ValueData AddSigned(ValueData left, ValueData right) {
  ValueData result{};
  // TODO(m4jr0): Define signed integer overflow semantics and avoid relying on
  // C++ signed overflow behavior.
  result.s64_value = static_cast<T>(static_cast<T>(left.s64_value) +
                                    static_cast<T>(right.s64_value));
  return result;
}

template <typename T>
ValueData AddUnsigned(ValueData left, ValueData right) {
  ValueData result{};
  result.u64_value = static_cast<T>(static_cast<T>(left.u64_value) +
                                    static_cast<T>(right.u64_value));
  return result;
}

template <typename T>
ValueData AddFloat(ValueData left, ValueData right) {
  ValueData result{};
  result.f64_value = static_cast<f64>(static_cast<T>(left.f64_value) +
                                      static_cast<T>(right.f64_value));
  return result;
}

template <typename T>
ValueData SubtractSigned(ValueData left, ValueData right) {
  ValueData result{};
  // TODO(m4jr0): Define signed integer overflow semantics and avoid relying on
  // C++ signed overflow behavior.
  result.s64_value = static_cast<T>(static_cast<T>(left.s64_value) -
                                    static_cast<T>(right.s64_value));
  return result;
}

template <typename T>
ValueData SubtractUnsigned(ValueData left, ValueData right) {
  ValueData result{};
  result.u64_value = static_cast<T>(static_cast<T>(left.u64_value) -
                                    static_cast<T>(right.u64_value));
  return result;
}

template <typename T>
ValueData SubtractFloat(ValueData left, ValueData right) {
  ValueData result{};
  result.f64_value = static_cast<f64>(static_cast<T>(left.f64_value) -
                                      static_cast<T>(right.f64_value));
  return result;
}

}  // namespace

std::optional<Value> Vm::Execute(const BytecodeModule& module) {
  FELL_ASSERT(module.register_count <= kMaxRegisterCount);

  auto get_register = [&](RegisterId id) -> ValueData& {
    FELL_ASSERT(id < module.register_count);
    return registers_[id];
  };

#define FELL_CONVERT_SIGNED(OPCODE, SOURCE, DESTINATION) \
  case Opcode::OPCODE:                                   \
    get_register(instruction.convert.destination) =      \
        ConvertSigned<SOURCE, DESTINATION>(              \
            get_register(instruction.convert.source));   \
    break

#define FELL_CONVERT_UNSIGNED(OPCODE, SOURCE, DESTINATION) \
  case Opcode::OPCODE:                                     \
    get_register(instruction.convert.destination) =        \
        ConvertUnsigned<SOURCE, DESTINATION>(              \
            get_register(instruction.convert.source));     \
    break

#define FELL_CONVERT_FLOAT(OPCODE, SOURCE, DESTINATION) \
  case Opcode::OPCODE:                                  \
    get_register(instruction.convert.destination) =     \
        ConvertFloat<SOURCE, DESTINATION>(              \
            get_register(instruction.convert.source));  \
    break

#define FELL_UNARY(OPCODE, FUNCTION, TYPE)                       \
  case Opcode::OPCODE:                                           \
    get_register(instruction.unary.destination) =                \
        FUNCTION<TYPE>(get_register(instruction.unary.operand)); \
    break

#define FELL_BINARY(OPCODE, FUNCTION, TYPE)                     \
  case Opcode::OPCODE:                                          \
    get_register(instruction.binary.destination) =              \
        FUNCTION<TYPE>(get_register(instruction.binary.left),   \
                       get_register(instruction.binary.right)); \
    break

  for (const Instruction& instruction : module.instructions) {
    switch (instruction.opcode) {
      case Opcode::kLoadImmediate:
        get_register(instruction.load_immediate.destination) =
            instruction.load_immediate.value.data;
        break;

        FELL_CONVERT_SIGNED(kConvertS8ToS16, s8, s16);
        FELL_CONVERT_SIGNED(kConvertS8ToS32, s8, s32);
        FELL_CONVERT_SIGNED(kConvertS8ToS64, s8, s64);
        FELL_CONVERT_SIGNED(kConvertS8ToF32, s8, f32);
        FELL_CONVERT_SIGNED(kConvertS8ToF64, s8, f64);
        FELL_CONVERT_SIGNED(kConvertS16ToS32, s16, s32);
        FELL_CONVERT_SIGNED(kConvertS16ToS64, s16, s64);
        FELL_CONVERT_SIGNED(kConvertS16ToF32, s16, f32);
        FELL_CONVERT_SIGNED(kConvertS16ToF64, s16, f64);
        FELL_CONVERT_SIGNED(kConvertS32ToS64, s32, s64);
        FELL_CONVERT_SIGNED(kConvertS32ToF64, s32, f64);
        FELL_CONVERT_UNSIGNED(kConvertU8ToS16, u8, s16);
        FELL_CONVERT_UNSIGNED(kConvertU8ToS32, u8, s32);
        FELL_CONVERT_UNSIGNED(kConvertU8ToS64, u8, s64);
        FELL_CONVERT_UNSIGNED(kConvertU8ToU16, u8, u16);
        FELL_CONVERT_UNSIGNED(kConvertU8ToU32, u8, u32);
        FELL_CONVERT_UNSIGNED(kConvertU8ToU64, u8, u64);
        FELL_CONVERT_UNSIGNED(kConvertU8ToF32, u8, f32);
        FELL_CONVERT_UNSIGNED(kConvertU8ToF64, u8, f64);
        FELL_CONVERT_UNSIGNED(kConvertU16ToS32, u16, s32);
        FELL_CONVERT_UNSIGNED(kConvertU16ToS64, u16, s64);
        FELL_CONVERT_UNSIGNED(kConvertU16ToU32, u16, u32);
        FELL_CONVERT_UNSIGNED(kConvertU16ToU64, u16, u64);
        FELL_CONVERT_UNSIGNED(kConvertU16ToF32, u16, f32);
        FELL_CONVERT_UNSIGNED(kConvertU16ToF64, u16, f64);
        FELL_CONVERT_UNSIGNED(kConvertU32ToS64, u32, s64);
        FELL_CONVERT_UNSIGNED(kConvertU32ToU64, u32, u64);
        FELL_CONVERT_UNSIGNED(kConvertU32ToF64, u32, f64);
        FELL_CONVERT_FLOAT(kConvertF32ToF64, f32, f64);

        FELL_UNARY(kNegateS8, NegateSigned, s8);
        FELL_UNARY(kNegateS16, NegateSigned, s16);
        FELL_UNARY(kNegateS32, NegateSigned, s32);
        FELL_UNARY(kNegateS64, NegateSigned, s64);
        FELL_UNARY(kNegateF32, NegateFloat, f32);
        FELL_UNARY(kNegateF64, NegateFloat, f64);

        FELL_BINARY(kMultiplyS8, MultiplySigned, s8);
        FELL_BINARY(kMultiplyS16, MultiplySigned, s16);
        FELL_BINARY(kMultiplyS32, MultiplySigned, s32);
        FELL_BINARY(kMultiplyS64, MultiplySigned, s64);
        FELL_BINARY(kMultiplyU8, MultiplyUnsigned, u8);
        FELL_BINARY(kMultiplyU16, MultiplyUnsigned, u16);
        FELL_BINARY(kMultiplyU32, MultiplyUnsigned, u32);
        FELL_BINARY(kMultiplyU64, MultiplyUnsigned, u64);
        FELL_BINARY(kMultiplyF32, MultiplyFloat, f32);
        FELL_BINARY(kMultiplyF64, MultiplyFloat, f64);

        FELL_BINARY(kDivideS8, DivideSigned, s8);
        FELL_BINARY(kDivideS16, DivideSigned, s16);
        FELL_BINARY(kDivideS32, DivideSigned, s32);
        FELL_BINARY(kDivideS64, DivideSigned, s64);
        FELL_BINARY(kDivideU8, DivideUnsigned, u8);
        FELL_BINARY(kDivideU16, DivideUnsigned, u16);
        FELL_BINARY(kDivideU32, DivideUnsigned, u32);
        FELL_BINARY(kDivideU64, DivideUnsigned, u64);
        FELL_BINARY(kDivideF32, DivideFloat, f32);
        FELL_BINARY(kDivideF64, DivideFloat, f64);

        FELL_BINARY(kAddS8, AddSigned, s8);
        FELL_BINARY(kAddS16, AddSigned, s16);
        FELL_BINARY(kAddS32, AddSigned, s32);
        FELL_BINARY(kAddS64, AddSigned, s64);
        FELL_BINARY(kAddU8, AddUnsigned, u8);
        FELL_BINARY(kAddU16, AddUnsigned, u16);
        FELL_BINARY(kAddU32, AddUnsigned, u32);
        FELL_BINARY(kAddU64, AddUnsigned, u64);
        FELL_BINARY(kAddF32, AddFloat, f32);
        FELL_BINARY(kAddF64, AddFloat, f64);

        FELL_BINARY(kSubtractS8, SubtractSigned, s8);
        FELL_BINARY(kSubtractS16, SubtractSigned, s16);
        FELL_BINARY(kSubtractS32, SubtractSigned, s32);
        FELL_BINARY(kSubtractS64, SubtractSigned, s64);
        FELL_BINARY(kSubtractU8, SubtractUnsigned, u8);
        FELL_BINARY(kSubtractU16, SubtractUnsigned, u16);
        FELL_BINARY(kSubtractU32, SubtractUnsigned, u32);
        FELL_BINARY(kSubtractU64, SubtractUnsigned, u64);
        FELL_BINARY(kSubtractF32, SubtractFloat, f32);
        FELL_BINARY(kSubtractF64, SubtractFloat, f64);

      case Opcode::kReturn:
        return Value{
            .type = instruction.return_.type,
            .data = get_register(instruction.return_.source),
        };
    }
  }

#undef FELL_UNARY
#undef FELL_BINARY
#undef FELL_CONVERT_FLOAT
#undef FELL_CONVERT_UNSIGNED
#undef FELL_CONVERT_SIGNED

  return std::nullopt;
}

}  // namespace fell

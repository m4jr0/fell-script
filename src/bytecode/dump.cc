#include "bytecode/dump.h"

#include <iomanip>
#include <sstream>

#include "core/assert.h"
#include "core/core.h"
#include "runtime/value.h"

namespace fell {

StringView ToString(Opcode opcode) {
  switch (opcode) {
    case Opcode::kLoadImmediate:
      return "load_immediate";
    case Opcode::kLoadString:
      return "load_string";
    case Opcode::kLoadGlobal:
      return "load_global";
    case Opcode::kStoreGlobal:
      return "store_global";
    case Opcode::kConvertS8ToS16:
      return "convert_s8_to_s16";
    case Opcode::kConvertS8ToS32:
      return "convert_s8_to_s32";
    case Opcode::kConvertS8ToS64:
      return "convert_s8_to_s64";
    case Opcode::kConvertS8ToF32:
      return "convert_s8_to_f32";
    case Opcode::kConvertS8ToF64:
      return "convert_s8_to_f64";
    case Opcode::kConvertS16ToS32:
      return "convert_s16_to_s32";
    case Opcode::kConvertS16ToS64:
      return "convert_s16_to_s64";
    case Opcode::kConvertS16ToF32:
      return "convert_s16_to_f32";
    case Opcode::kConvertS16ToF64:
      return "convert_s16_to_f64";
    case Opcode::kConvertS32ToS64:
      return "convert_s32_to_s64";
    case Opcode::kConvertS32ToF64:
      return "convert_s32_to_f64";
    case Opcode::kConvertU8ToS16:
      return "convert_u8_to_s16";
    case Opcode::kConvertU8ToS32:
      return "convert_u8_to_s32";
    case Opcode::kConvertU8ToS64:
      return "convert_u8_to_s64";
    case Opcode::kConvertU8ToU16:
      return "convert_u8_to_u16";
    case Opcode::kConvertU8ToU32:
      return "convert_u8_to_u32";
    case Opcode::kConvertU8ToU64:
      return "convert_u8_to_u64";
    case Opcode::kConvertU8ToF32:
      return "convert_u8_to_f32";
    case Opcode::kConvertU8ToF64:
      return "convert_u8_to_f64";
    case Opcode::kConvertU16ToS32:
      return "convert_u16_to_s32";
    case Opcode::kConvertU16ToS64:
      return "convert_u16_to_s64";
    case Opcode::kConvertU16ToU32:
      return "convert_u16_to_u32";
    case Opcode::kConvertU16ToU64:
      return "convert_u16_to_u64";
    case Opcode::kConvertU16ToF32:
      return "convert_u16_to_f32";
    case Opcode::kConvertU16ToF64:
      return "convert_u16_to_f64";
    case Opcode::kConvertU32ToS64:
      return "convert_u32_to_s64";
    case Opcode::kConvertU32ToU64:
      return "convert_u32_to_u64";
    case Opcode::kConvertU32ToF64:
      return "convert_u32_to_f64";
    case Opcode::kConvertF32ToF64:
      return "convert_f32_to_f64";
    case Opcode::kNegateS8:
      return "negate_s8";
    case Opcode::kNegateS16:
      return "negate_s16";
    case Opcode::kNegateS32:
      return "negate_s32";
    case Opcode::kNegateS64:
      return "negate_s64";
    case Opcode::kNegateF32:
      return "negate_f32";
    case Opcode::kNegateF64:
      return "negate_f64";
    case Opcode::kLogicalNot:
      return "logical_not";
    case Opcode::kMultiplyS8:
      return "multiply_s8";
    case Opcode::kMultiplyS16:
      return "multiply_s16";
    case Opcode::kMultiplyS32:
      return "multiply_s32";
    case Opcode::kMultiplyS64:
      return "multiply_s64";
    case Opcode::kMultiplyU8:
      return "multiply_u8";
    case Opcode::kMultiplyU16:
      return "multiply_u16";
    case Opcode::kMultiplyU32:
      return "multiply_u32";
    case Opcode::kMultiplyU64:
      return "multiply_u64";
    case Opcode::kMultiplyF32:
      return "multiply_f32";
    case Opcode::kMultiplyF64:
      return "multiply_f64";
    case Opcode::kDivideS8:
      return "divide_s8";
    case Opcode::kDivideS16:
      return "divide_s16";
    case Opcode::kDivideS32:
      return "divide_s32";
    case Opcode::kDivideS64:
      return "divide_s64";
    case Opcode::kDivideU8:
      return "divide_u8";
    case Opcode::kDivideU16:
      return "divide_u16";
    case Opcode::kDivideU32:
      return "divide_u32";
    case Opcode::kDivideU64:
      return "divide_u64";
    case Opcode::kDivideF32:
      return "divide_f32";
    case Opcode::kDivideF64:
      return "divide_f64";
    case Opcode::kAddS8:
      return "add_s8";
    case Opcode::kAddS16:
      return "add_s16";
    case Opcode::kAddS32:
      return "add_s32";
    case Opcode::kAddS64:
      return "add_s64";
    case Opcode::kAddU8:
      return "add_u8";
    case Opcode::kAddU16:
      return "add_u16";
    case Opcode::kAddU32:
      return "add_u32";
    case Opcode::kAddU64:
      return "add_u64";
    case Opcode::kAddF32:
      return "add_f32";
    case Opcode::kAddF64:
      return "add_f64";
    case Opcode::kAddString:
      return "add_string";
    case Opcode::kSubtractS8:
      return "subtract_s8";
    case Opcode::kSubtractS16:
      return "subtract_s16";
    case Opcode::kSubtractS32:
      return "subtract_s32";
    case Opcode::kSubtractS64:
      return "subtract_s64";
    case Opcode::kSubtractU8:
      return "subtract_u8";
    case Opcode::kSubtractU16:
      return "subtract_u16";
    case Opcode::kSubtractU32:
      return "subtract_u32";
    case Opcode::kSubtractU64:
      return "subtract_u64";
    case Opcode::kSubtractF32:
      return "subtract_f32";
    case Opcode::kSubtractF64:
      return "subtract_f64";
    case Opcode::kEqualBool:
      return "equal_bool";
    case Opcode::kEqualS8:
      return "equal_s8";
    case Opcode::kEqualS16:
      return "equal_s16";
    case Opcode::kEqualS32:
      return "equal_s32";
    case Opcode::kEqualS64:
      return "equal_s64";
    case Opcode::kEqualU8:
      return "equal_u8";
    case Opcode::kEqualU16:
      return "equal_u16";
    case Opcode::kEqualU32:
      return "equal_u32";
    case Opcode::kEqualU64:
      return "equal_u64";
    case Opcode::kEqualF32:
      return "equal_f32";
    case Opcode::kEqualF64:
      return "equal_f64";
    case Opcode::kEqualString:
      return "equal_string";
    case Opcode::kNotEqualBool:
      return "not_equal_bool";
    case Opcode::kNotEqualS8:
      return "not_equal_s8";
    case Opcode::kNotEqualS16:
      return "not_equal_s16";
    case Opcode::kNotEqualS32:
      return "not_equal_s32";
    case Opcode::kNotEqualS64:
      return "not_equal_s64";
    case Opcode::kNotEqualU8:
      return "not_equal_u8";
    case Opcode::kNotEqualU16:
      return "not_equal_u16";
    case Opcode::kNotEqualU32:
      return "not_equal_u32";
    case Opcode::kNotEqualU64:
      return "not_equal_u64";
    case Opcode::kNotEqualF32:
      return "not_equal_f32";
    case Opcode::kNotEqualF64:
      return "not_equal_f64";
    case Opcode::kNotEqualString:
      return "not_equal_string";
    case Opcode::kLessS8:
      return "less_s8";
    case Opcode::kLessS16:
      return "less_s16";
    case Opcode::kLessS32:
      return "less_s32";
    case Opcode::kLessS64:
      return "less_s64";
    case Opcode::kLessU8:
      return "less_u8";
    case Opcode::kLessU16:
      return "less_u16";
    case Opcode::kLessU32:
      return "less_u32";
    case Opcode::kLessU64:
      return "less_u64";
    case Opcode::kLessF32:
      return "less_f32";
    case Opcode::kLessF64:
      return "less_f64";
    case Opcode::kLessEqualS8:
      return "less_equal_s8";
    case Opcode::kLessEqualS16:
      return "less_equal_s16";
    case Opcode::kLessEqualS32:
      return "less_equal_s32";
    case Opcode::kLessEqualS64:
      return "less_equal_s64";
    case Opcode::kLessEqualU8:
      return "less_equal_u8";
    case Opcode::kLessEqualU16:
      return "less_equal_u16";
    case Opcode::kLessEqualU32:
      return "less_equal_u32";
    case Opcode::kLessEqualU64:
      return "less_equal_u64";
    case Opcode::kLessEqualF32:
      return "less_equal_f32";
    case Opcode::kLessEqualF64:
      return "less_equal_f64";
    case Opcode::kGreaterS8:
      return "greater_s8";
    case Opcode::kGreaterS16:
      return "greater_s16";
    case Opcode::kGreaterS32:
      return "greater_s32";
    case Opcode::kGreaterS64:
      return "greater_s64";
    case Opcode::kGreaterU8:
      return "greater_u8";
    case Opcode::kGreaterU16:
      return "greater_u16";
    case Opcode::kGreaterU32:
      return "greater_u32";
    case Opcode::kGreaterU64:
      return "greater_u64";
    case Opcode::kGreaterF32:
      return "greater_f32";
    case Opcode::kGreaterF64:
      return "greater_f64";
    case Opcode::kGreaterEqualS8:
      return "greater_equal_s8";
    case Opcode::kGreaterEqualS16:
      return "greater_equal_s16";
    case Opcode::kGreaterEqualS32:
      return "greater_equal_s32";
    case Opcode::kGreaterEqualS64:
      return "greater_equal_s64";
    case Opcode::kGreaterEqualU8:
      return "greater_equal_u8";
    case Opcode::kGreaterEqualU16:
      return "greater_equal_u16";
    case Opcode::kGreaterEqualU32:
      return "greater_equal_u32";
    case Opcode::kGreaterEqualU64:
      return "greater_equal_u64";
    case Opcode::kGreaterEqualF32:
      return "greater_equal_f32";
    case Opcode::kGreaterEqualF64:
      return "greater_equal_f64";
    case Opcode::kReturn:
      return "return";
  }

  FELL_UNREACHABLE();
}

String DumpBytecode(const BytecodeModule& module) {
  std::ostringstream output{};
  output << "registers: " << module.register_count << "\n\n";

  for (usize index{0}; index < module.instructions.size(); ++index) {
    const Instruction& instruction{module.instructions[index]};

    output << std::setw(4) << std::setfill('0') << index << "  "
           << ToString(instruction.opcode);

    switch (instruction.opcode) {
      case Opcode::kLoadImmediate:
        output << " r" << instruction.load_immediate.destination << ", "
               << ToString(instruction.load_immediate.value);
        break;

      case Opcode::kLoadString:
        output << " r" << instruction.load_string.destination << ", #"
               << instruction.load_string.constant;
        break;

      case Opcode::kLoadGlobal:
        output << " r" << instruction.global.value << ", #"
               << instruction.global.global;
        break;

      case Opcode::kStoreGlobal:
        output << " #" << instruction.global.global << ", r"
               << instruction.global.value;
        break;

      case Opcode::kConvertS8ToS16:
      case Opcode::kConvertS8ToS32:
      case Opcode::kConvertS8ToS64:
      case Opcode::kConvertS8ToF32:
      case Opcode::kConvertS8ToF64:
      case Opcode::kConvertS16ToS32:
      case Opcode::kConvertS16ToS64:
      case Opcode::kConvertS16ToF32:
      case Opcode::kConvertS16ToF64:
      case Opcode::kConvertS32ToS64:
      case Opcode::kConvertS32ToF64:
      case Opcode::kConvertU8ToS16:
      case Opcode::kConvertU8ToS32:
      case Opcode::kConvertU8ToS64:
      case Opcode::kConvertU8ToU16:
      case Opcode::kConvertU8ToU32:
      case Opcode::kConvertU8ToU64:
      case Opcode::kConvertU8ToF32:
      case Opcode::kConvertU8ToF64:
      case Opcode::kConvertU16ToS32:
      case Opcode::kConvertU16ToS64:
      case Opcode::kConvertU16ToU32:
      case Opcode::kConvertU16ToU64:
      case Opcode::kConvertU16ToF32:
      case Opcode::kConvertU16ToF64:
      case Opcode::kConvertU32ToS64:
      case Opcode::kConvertU32ToU64:
      case Opcode::kConvertU32ToF64:
      case Opcode::kConvertF32ToF64:
        output << " r" << instruction.convert.destination << ", r"
               << instruction.convert.source;
        break;

      case Opcode::kNegateS8:
      case Opcode::kNegateS16:
      case Opcode::kNegateS32:
      case Opcode::kNegateS64:
      case Opcode::kNegateF32:
      case Opcode::kNegateF64:
      case Opcode::kLogicalNot:
        output << " r" << instruction.unary.destination << ", r"
               << instruction.unary.operand;
        break;

      case Opcode::kMultiplyS8:
      case Opcode::kMultiplyS16:
      case Opcode::kMultiplyS32:
      case Opcode::kMultiplyS64:
      case Opcode::kMultiplyU8:
      case Opcode::kMultiplyU16:
      case Opcode::kMultiplyU32:
      case Opcode::kMultiplyU64:
      case Opcode::kMultiplyF32:
      case Opcode::kMultiplyF64:
      case Opcode::kDivideS8:
      case Opcode::kDivideS16:
      case Opcode::kDivideS32:
      case Opcode::kDivideS64:
      case Opcode::kDivideU8:
      case Opcode::kDivideU16:
      case Opcode::kDivideU32:
      case Opcode::kDivideU64:
      case Opcode::kDivideF32:
      case Opcode::kDivideF64:
      case Opcode::kAddS8:
      case Opcode::kAddS16:
      case Opcode::kAddS32:
      case Opcode::kAddS64:
      case Opcode::kAddU8:
      case Opcode::kAddU16:
      case Opcode::kAddU32:
      case Opcode::kAddU64:
      case Opcode::kAddF32:
      case Opcode::kAddF64:
      case Opcode::kSubtractS8:
      case Opcode::kSubtractS16:
      case Opcode::kSubtractS32:
      case Opcode::kSubtractS64:
      case Opcode::kSubtractU8:
      case Opcode::kSubtractU16:
      case Opcode::kSubtractU32:
      case Opcode::kSubtractU64:
      case Opcode::kSubtractF32:
      case Opcode::kSubtractF64:
      case Opcode::kEqualBool:
      case Opcode::kEqualS8:
      case Opcode::kEqualS16:
      case Opcode::kEqualS32:
      case Opcode::kEqualS64:
      case Opcode::kEqualU8:
      case Opcode::kEqualU16:
      case Opcode::kEqualU32:
      case Opcode::kEqualU64:
      case Opcode::kEqualF32:
      case Opcode::kEqualF64:
      case Opcode::kNotEqualBool:
      case Opcode::kNotEqualS8:
      case Opcode::kNotEqualS16:
      case Opcode::kNotEqualS32:
      case Opcode::kNotEqualS64:
      case Opcode::kNotEqualU8:
      case Opcode::kNotEqualU16:
      case Opcode::kNotEqualU32:
      case Opcode::kNotEqualU64:
      case Opcode::kNotEqualF32:
      case Opcode::kNotEqualF64:
      case Opcode::kLessS8:
      case Opcode::kLessS16:
      case Opcode::kLessS32:
      case Opcode::kLessS64:
      case Opcode::kLessU8:
      case Opcode::kLessU16:
      case Opcode::kLessU32:
      case Opcode::kLessU64:
      case Opcode::kLessF32:
      case Opcode::kLessF64:
      case Opcode::kLessEqualS8:
      case Opcode::kLessEqualS16:
      case Opcode::kLessEqualS32:
      case Opcode::kLessEqualS64:
      case Opcode::kLessEqualU8:
      case Opcode::kLessEqualU16:
      case Opcode::kLessEqualU32:
      case Opcode::kLessEqualU64:
      case Opcode::kLessEqualF32:
      case Opcode::kLessEqualF64:
      case Opcode::kGreaterS8:
      case Opcode::kGreaterS16:
      case Opcode::kGreaterS32:
      case Opcode::kGreaterS64:
      case Opcode::kGreaterU8:
      case Opcode::kGreaterU16:
      case Opcode::kGreaterU32:
      case Opcode::kGreaterU64:
      case Opcode::kGreaterF32:
      case Opcode::kGreaterF64:
      case Opcode::kGreaterEqualS8:
      case Opcode::kGreaterEqualS16:
      case Opcode::kGreaterEqualS32:
      case Opcode::kGreaterEqualS64:
      case Opcode::kGreaterEqualU8:
      case Opcode::kGreaterEqualU16:
      case Opcode::kGreaterEqualU32:
      case Opcode::kGreaterEqualU64:
      case Opcode::kGreaterEqualF32:
      case Opcode::kGreaterEqualF64:
      case Opcode::kAddString:
      case Opcode::kEqualString:
      case Opcode::kNotEqualString:
        output << " r" << instruction.binary.destination << ", r"
               << instruction.binary.left << ", r" << instruction.binary.right;
        break;

      case Opcode::kReturn:
        output << " r" << instruction.return_.source;
        break;
    }

    output << '\n';
  }

  return output.str();
}

}  // namespace fell

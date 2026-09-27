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

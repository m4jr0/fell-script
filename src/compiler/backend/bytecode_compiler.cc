#include "compiler/backend/bytecode_compiler.h"

#include <algorithm>
#include <utility>

#include "core/assert.h"
#include "core/type.h"

namespace fell {
namespace {

RegisterId ToRegister(IrValueId value) {
  FELL_ASSERT(value.value < kMaxRegisterCount);
  return static_cast<RegisterId>(value.value);
}

ValueType ToValueType(Type type) {
  switch (type) {
    case Type::kBool:
      return ValueType::kBool;
    case Type::kS8:
      return ValueType::kS8;
    case Type::kS16:
      return ValueType::kS16;
    case Type::kS32:
      return ValueType::kS32;
    case Type::kS64:
      return ValueType::kS64;
    case Type::kU8:
      return ValueType::kU8;
    case Type::kU16:
      return ValueType::kU16;
    case Type::kU32:
      return ValueType::kU32;
    case Type::kU64:
      return ValueType::kU64;
    case Type::kF32:
      return ValueType::kF32;
    case Type::kF64:
      return ValueType::kF64;
    case Type::kString:
      return ValueType::kString;

    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

Opcode GetConvertOpcode(Type source, Type destination) {
  FELL_ASSERT(source != destination);
  FELL_ASSERT(CanImplicitlyConvert(source, destination));

  switch (source) {
    case Type::kS8:
      switch (destination) {
        case Type::kS16:
          return Opcode::kConvertS8ToS16;
        case Type::kS32:
          return Opcode::kConvertS8ToS32;
        case Type::kS64:
          return Opcode::kConvertS8ToS64;
        case Type::kF32:
          return Opcode::kConvertS8ToF32;
        case Type::kF64:
          return Opcode::kConvertS8ToF64;

        default:
          FELL_UNREACHABLE();
      }

    case Type::kS16:
      switch (destination) {
        case Type::kS32:
          return Opcode::kConvertS16ToS32;
        case Type::kS64:
          return Opcode::kConvertS16ToS64;
        case Type::kF32:
          return Opcode::kConvertS16ToF32;
        case Type::kF64:
          return Opcode::kConvertS16ToF64;

        default:
          FELL_UNREACHABLE();
      }

    case Type::kS32:
      switch (destination) {
        case Type::kS64:
          return Opcode::kConvertS32ToS64;
        case Type::kF64:
          return Opcode::kConvertS32ToF64;

        default:
          FELL_UNREACHABLE();
      }

    case Type::kU8:
      switch (destination) {
        case Type::kS16:
          return Opcode::kConvertU8ToS16;
        case Type::kS32:
          return Opcode::kConvertU8ToS32;
        case Type::kS64:
          return Opcode::kConvertU8ToS64;
        case Type::kU16:
          return Opcode::kConvertU8ToU16;
        case Type::kU32:
          return Opcode::kConvertU8ToU32;
        case Type::kU64:
          return Opcode::kConvertU8ToU64;
        case Type::kF32:
          return Opcode::kConvertU8ToF32;
        case Type::kF64:
          return Opcode::kConvertU8ToF64;

        default:
          FELL_UNREACHABLE();
      }

    case Type::kU16:
      switch (destination) {
        case Type::kS32:
          return Opcode::kConvertU16ToS32;
        case Type::kS64:
          return Opcode::kConvertU16ToS64;
        case Type::kU32:
          return Opcode::kConvertU16ToU32;
        case Type::kU64:
          return Opcode::kConvertU16ToU64;
        case Type::kF32:
          return Opcode::kConvertU16ToF32;
        case Type::kF64:
          return Opcode::kConvertU16ToF64;

        default:
          FELL_UNREACHABLE();
      }

    case Type::kU32:
      switch (destination) {
        case Type::kS64:
          return Opcode::kConvertU32ToS64;
        case Type::kU64:
          return Opcode::kConvertU32ToU64;
        case Type::kF64:
          return Opcode::kConvertU32ToF64;

        default:
          FELL_UNREACHABLE();
      }

    case Type::kF32:
      FELL_ASSERT(destination == Type::kF64);
      return Opcode::kConvertF32ToF64;

    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
    case Type::kBool:
    case Type::kString:
    case Type::kS64:
    case Type::kU64:
    case Type::kF64:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

Opcode GetMultiplyOpcode(Type type) {
  switch (type) {
    case Type::kS8:
      return Opcode::kMultiplyS8;
    case Type::kS16:
      return Opcode::kMultiplyS16;
    case Type::kS32:
      return Opcode::kMultiplyS32;
    case Type::kS64:
      return Opcode::kMultiplyS64;
    case Type::kU8:
      return Opcode::kMultiplyU8;
    case Type::kU16:
      return Opcode::kMultiplyU16;
    case Type::kU32:
      return Opcode::kMultiplyU32;
    case Type::kU64:
      return Opcode::kMultiplyU64;
    case Type::kF32:
      return Opcode::kMultiplyF32;
    case Type::kF64:
      return Opcode::kMultiplyF64;

    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
    case Type::kBool:
    case Type::kString:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

Opcode GetDivideOpcode(Type type) {
  switch (type) {
    case Type::kS8:
      return Opcode::kDivideS8;
    case Type::kS16:
      return Opcode::kDivideS16;
    case Type::kS32:
      return Opcode::kDivideS32;
    case Type::kS64:
      return Opcode::kDivideS64;
    case Type::kU8:
      return Opcode::kDivideU8;
    case Type::kU16:
      return Opcode::kDivideU16;
    case Type::kU32:
      return Opcode::kDivideU32;
    case Type::kU64:
      return Opcode::kDivideU64;
    case Type::kF32:
      return Opcode::kDivideF32;
    case Type::kF64:
      return Opcode::kDivideF64;

    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
    case Type::kBool:
    case Type::kString:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

Opcode GetAddOpcode(Type type) {
  switch (type) {
    case Type::kS8:
      return Opcode::kAddS8;
    case Type::kS16:
      return Opcode::kAddS16;
    case Type::kS32:
      return Opcode::kAddS32;
    case Type::kS64:
      return Opcode::kAddS64;
    case Type::kU8:
      return Opcode::kAddU8;
    case Type::kU16:
      return Opcode::kAddU16;
    case Type::kU32:
      return Opcode::kAddU32;
    case Type::kU64:
      return Opcode::kAddU64;
    case Type::kF32:
      return Opcode::kAddF32;
    case Type::kF64:
      return Opcode::kAddF64;
    case Type::kString:
      return Opcode::kAddString;

    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
    case Type::kBool:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

Opcode GetSubtractOpcode(Type type) {
  switch (type) {
    case Type::kS8:
      return Opcode::kSubtractS8;
    case Type::kS16:
      return Opcode::kSubtractS16;
    case Type::kS32:
      return Opcode::kSubtractS32;
    case Type::kS64:
      return Opcode::kSubtractS64;
    case Type::kU8:
      return Opcode::kSubtractU8;
    case Type::kU16:
      return Opcode::kSubtractU16;
    case Type::kU32:
      return Opcode::kSubtractU32;
    case Type::kU64:
      return Opcode::kSubtractU64;
    case Type::kF32:
      return Opcode::kSubtractF32;
    case Type::kF64:
      return Opcode::kSubtractF64;

    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
    case Type::kBool:
    case Type::kString:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

Opcode GetComparisonOpcode(IrOpcode opcode, Type type) {
  switch (opcode) {
    case IrOpcode::kEqual:
      switch (type) {
        case Type::kBool:
          return Opcode::kEqualBool;
        case Type::kS8:
          return Opcode::kEqualS8;
        case Type::kS16:
          return Opcode::kEqualS16;
        case Type::kS32:
          return Opcode::kEqualS32;
        case Type::kS64:
          return Opcode::kEqualS64;
        case Type::kU8:
          return Opcode::kEqualU8;
        case Type::kU16:
          return Opcode::kEqualU16;
        case Type::kU32:
          return Opcode::kEqualU32;
        case Type::kU64:
          return Opcode::kEqualU64;
        case Type::kF32:
          return Opcode::kEqualF32;
        case Type::kF64:
          return Opcode::kEqualF64;
        case Type::kString:
          return Opcode::kEqualString;
        default:
          FELL_UNREACHABLE();
      }
    case IrOpcode::kNotEqual:
      switch (type) {
        case Type::kBool:
          return Opcode::kNotEqualBool;
        case Type::kS8:
          return Opcode::kNotEqualS8;
        case Type::kS16:
          return Opcode::kNotEqualS16;
        case Type::kS32:
          return Opcode::kNotEqualS32;
        case Type::kS64:
          return Opcode::kNotEqualS64;
        case Type::kU8:
          return Opcode::kNotEqualU8;
        case Type::kU16:
          return Opcode::kNotEqualU16;
        case Type::kU32:
          return Opcode::kNotEqualU32;
        case Type::kU64:
          return Opcode::kNotEqualU64;
        case Type::kF32:
          return Opcode::kNotEqualF32;
        case Type::kF64:
          return Opcode::kNotEqualF64;
        case Type::kString:
          return Opcode::kNotEqualString;
        default:
          FELL_UNREACHABLE();
      }
    case IrOpcode::kLess:
      switch (type) {
        case Type::kS8:
          return Opcode::kLessS8;
        case Type::kS16:
          return Opcode::kLessS16;
        case Type::kS32:
          return Opcode::kLessS32;
        case Type::kS64:
          return Opcode::kLessS64;
        case Type::kU8:
          return Opcode::kLessU8;
        case Type::kU16:
          return Opcode::kLessU16;
        case Type::kU32:
          return Opcode::kLessU32;
        case Type::kU64:
          return Opcode::kLessU64;
        case Type::kF32:
          return Opcode::kLessF32;
        case Type::kF64:
          return Opcode::kLessF64;
        default:
          FELL_UNREACHABLE();
      }
    case IrOpcode::kLessEqual:
      switch (type) {
        case Type::kS8:
          return Opcode::kLessEqualS8;
        case Type::kS16:
          return Opcode::kLessEqualS16;
        case Type::kS32:
          return Opcode::kLessEqualS32;
        case Type::kS64:
          return Opcode::kLessEqualS64;
        case Type::kU8:
          return Opcode::kLessEqualU8;
        case Type::kU16:
          return Opcode::kLessEqualU16;
        case Type::kU32:
          return Opcode::kLessEqualU32;
        case Type::kU64:
          return Opcode::kLessEqualU64;
        case Type::kF32:
          return Opcode::kLessEqualF32;
        case Type::kF64:
          return Opcode::kLessEqualF64;
        default:
          FELL_UNREACHABLE();
      }
    case IrOpcode::kGreater:
      switch (type) {
        case Type::kS8:
          return Opcode::kGreaterS8;
        case Type::kS16:
          return Opcode::kGreaterS16;
        case Type::kS32:
          return Opcode::kGreaterS32;
        case Type::kS64:
          return Opcode::kGreaterS64;
        case Type::kU8:
          return Opcode::kGreaterU8;
        case Type::kU16:
          return Opcode::kGreaterU16;
        case Type::kU32:
          return Opcode::kGreaterU32;
        case Type::kU64:
          return Opcode::kGreaterU64;
        case Type::kF32:
          return Opcode::kGreaterF32;
        case Type::kF64:
          return Opcode::kGreaterF64;
        default:
          FELL_UNREACHABLE();
      }
    case IrOpcode::kGreaterEqual:
      switch (type) {
        case Type::kS8:
          return Opcode::kGreaterEqualS8;
        case Type::kS16:
          return Opcode::kGreaterEqualS16;
        case Type::kS32:
          return Opcode::kGreaterEqualS32;
        case Type::kS64:
          return Opcode::kGreaterEqualS64;
        case Type::kU8:
          return Opcode::kGreaterEqualU8;
        case Type::kU16:
          return Opcode::kGreaterEqualU16;
        case Type::kU32:
          return Opcode::kGreaterEqualU32;
        case Type::kU64:
          return Opcode::kGreaterEqualU64;
        case Type::kF32:
          return Opcode::kGreaterEqualF32;
        case Type::kF64:
          return Opcode::kGreaterEqualF64;
        default:
          FELL_UNREACHABLE();
      }
    default:
      FELL_UNREACHABLE();
  }
}

Value MakeConstantValue(const IrProcedure& procedure,
                        const IrConstant& constant) {
  const Type type{GetIrValue(procedure, constant.destination).type};

  Value value{
      .type = ToValueType(type),
      .data = {},
  };

  switch (type) {
    case Type::kBool:
      value.data.bool_value = constant.bool_value;
      break;

    case Type::kS8:
    case Type::kS16:
    case Type::kS32:
    case Type::kS64:
      value.data.s64_value = constant.s64_value;
      break;

    case Type::kU8:
    case Type::kU16:
    case Type::kU32:
    case Type::kU64:
      value.data.u64_value = constant.u64_value;
      break;

    case Type::kF32:
    case Type::kF64:
      value.data.f64_value = constant.f64_value;
      break;

    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
    case Type::kString:
      FELL_UNREACHABLE();
  }

  return value;
}

Opcode GetNegateOpcode(Type type) {
  switch (type) {
    case Type::kS8:
      return Opcode::kNegateS8;
    case Type::kS16:
      return Opcode::kNegateS16;
    case Type::kS32:
      return Opcode::kNegateS32;
    case Type::kS64:
      return Opcode::kNegateS64;
    case Type::kF32:
      return Opcode::kNegateF32;
    case Type::kF64:
      return Opcode::kNegateF64;

    default:
      FELL_UNREACHABLE();
  }
}

Opcode GetUnaryOpcode(IrOpcode opcode, Type type) {
  switch (opcode) {
    case IrOpcode::kNegate:
      return GetNegateOpcode(type);
    case IrOpcode::kLogicalNot:
      FELL_ASSERT(type == Type::kBool);
      return Opcode::kLogicalNot;

    default:
      FELL_UNREACHABLE();
  }
}

Opcode GetBinaryOpcode(IrOpcode opcode, Type type) {
  switch (opcode) {
    case IrOpcode::kMultiply:
      return GetMultiplyOpcode(type);
    case IrOpcode::kDivide:
      return GetDivideOpcode(type);
    case IrOpcode::kAdd:
      return GetAddOpcode(type);
    case IrOpcode::kSubtract:
      return GetSubtractOpcode(type);

    default:
      FELL_UNREACHABLE();
  }
}

u32 GetInstructionCount(const IrInstruction& instruction) {
  FELL_ASSERT(instruction.opcode != IrOpcode::kInvalid);
  return 1;
}

u32 GetTerminatorInstructionCount(const IrTerminator& terminator) {
  switch (terminator.kind) {
    case IrTerminatorKind::kInvalid:
      FELL_UNREACHABLE();

    case IrTerminatorKind::kJump:
    case IrTerminatorKind::kReturn:
    case IrTerminatorKind::kExit:
      return 1;

    case IrTerminatorKind::kBranch:
      return 2;
  }

  FELL_UNREACHABLE();
}

u16 GetRegisterCount(const IrProgram& ir) {
  usize max_value_count{ir.main.values.size()};

  for (const IrFunction& function : ir.functions) {
    max_value_count =
        std::max(max_value_count, function.procedure.values.size());
  }

  FELL_ASSERT(max_value_count <= kMaxRegisterCount);
  return static_cast<u16>(max_value_count);
}

u32 GetLocalCount(const IrProgram& ir) {
  u32 local_count{ir.main.local_count};

  for (const IrFunction& function : ir.functions) {
    local_count = std::max(local_count, function.procedure.local_count);
  }

  return local_count;
}

}  // namespace

BytecodeCompiler::ProcedureLayout BytecodeCompiler::ComputeProcedureLayout(
    const IrProcedure& procedure, u32 start) const {
  ProcedureLayout layout{
      .block_targets = Vector<u32>(procedure.blocks.size()),
      .end = 0,
  };

  u32 bytecode_index{start};

  for (usize index{0}; index < procedure.blocks.size(); ++index) {
    const IrBasicBlock& block{procedure.blocks[index]};

    layout.block_targets[index] = bytecode_index;

    for (const IrInstruction& instruction : block.instructions) {
      bytecode_index += GetInstructionCount(instruction);
    }

    bytecode_index += GetTerminatorInstructionCount(block.terminator);
  }

  layout.end = bytecode_index;
  return layout;
}

BytecodeCompiler::ProcedureLayout BytecodeCompiler::CompileProcedure(
    const IrProcedure& procedure, BytecodeModule& module) {
  const u32 procedure_start{static_cast<u32>(module.instructions.size())};
  const ProcedureLayout layout{
      ComputeProcedureLayout(procedure, procedure_start),
  };
  const u32 argument_base{static_cast<u32>(module.call_arguments.size())};

  for (IrValueId argument : procedure.call_arguments) {
    module.call_arguments.push_back(ToRegister(argument));
  }

  for (const IrBasicBlock& block : procedure.blocks) {
    CompileBlock(procedure, block, layout, argument_base, module);
  }

  FELL_ASSERT(module.instructions.size() == layout.end);
  return layout;
}

void BytecodeCompiler::CompileBlock(const IrProcedure& procedure,
                                    const IrBasicBlock& block,
                                    const ProcedureLayout& layout,
                                    u32 argument_base, BytecodeModule& module) {
  for (const IrInstruction& instruction : block.instructions) {
    CompileInstruction(procedure, instruction, argument_base, module);
  }

  CompileTerminator(procedure, block.terminator, layout, module);
}

void BytecodeCompiler::CompileInstruction(const IrProcedure& procedure,
                                          const IrInstruction& instruction,
                                          u32 argument_base,
                                          BytecodeModule& module) {
  switch (instruction.opcode) {
    case IrOpcode::kInvalid:
      FELL_UNREACHABLE();

    case IrOpcode::kConstant: {
      const Type type{
          GetIrValue(procedure, instruction.constant.destination).type,
      };

      if (type == Type::kString) {
        module.instructions.push_back({
            .opcode = Opcode::kLoadString,
            .load_string =
                {
                    .destination = ToRegister(instruction.constant.destination),
                    .constant = instruction.constant.string_value,
                },
        });
      } else {
        module.instructions.push_back({
            .opcode = Opcode::kLoadImmediate,
            .load_immediate =
                {
                    .destination = ToRegister(instruction.constant.destination),
                    .value = MakeConstantValue(procedure, instruction.constant),
                },
        });
      }

      break;
    }

    case IrOpcode::kLoadGlobal:
      module.instructions.push_back({
          .opcode = Opcode::kLoadGlobal,
          .global =
              {
                  .value = ToRegister(instruction.global.value),
                  .global = instruction.global.global,
              },
      });

      break;

    case IrOpcode::kStoreGlobal:
      module.instructions.push_back({
          .opcode = Opcode::kStoreGlobal,
          .global =
              {
                  .value = ToRegister(instruction.global.value),
                  .global = instruction.global.global,
              },
      });

      break;

    case IrOpcode::kLoadLocal:
      module.instructions.push_back({
          .opcode = Opcode::kLoadLocal,
          .local =
              {
                  .value = ToRegister(instruction.local.value),
                  .local = instruction.local.local,
              },
      });

      break;

    case IrOpcode::kStoreLocal:
      module.instructions.push_back({
          .opcode = Opcode::kStoreLocal,
          .local =
              {
                  .value = ToRegister(instruction.local.value),
                  .local = instruction.local.local,
              },
      });

      break;

    case IrOpcode::kMove:
      module.instructions.push_back({
          .opcode = Opcode::kMove,
          .move =
              {
                  .destination = ToRegister(instruction.move.destination),
                  .source = ToRegister(instruction.move.source),
              },
      });

      break;

    case IrOpcode::kCall:
      module.instructions.push_back({
          .opcode = Opcode::kCall,
          .call =
              {
                  .destination = ToRegister(instruction.call.destination),
                  .function = instruction.call.function,
                  .argument_offset =
                      argument_base + instruction.call.argument_offset,
                  .argument_count = instruction.call.argument_count,
              },
      });

      break;

    case IrOpcode::kCallNative:
      module.instructions.push_back({
          .opcode = Opcode::kCallNative,
          .call_native =
              {
                  .argument = ToRegister(instruction.call_native.argument),
                  .type = ToValueType(instruction.call_native.argument_type),
                  .function = instruction.call_native.function,
              },
      });

      break;

    case IrOpcode::kConvert: {
      const Type source_type{
          GetIrValue(procedure, instruction.convert.source).type,
      };
      const Type destination_type{
          GetIrValue(procedure, instruction.convert.destination).type,
      };

      module.instructions.push_back({
          .opcode = GetConvertOpcode(source_type, destination_type),
          .convert =
              {
                  .destination = ToRegister(instruction.convert.destination),
                  .source = ToRegister(instruction.convert.source),
              },
      });

      break;
    }

    case IrOpcode::kNegate:
    case IrOpcode::kLogicalNot: {
      const Type type{
          GetIrValue(procedure, instruction.unary.destination).type,
      };
      FELL_ASSERT(GetIrValue(procedure, instruction.unary.operand).type ==
                  type);

      module.instructions.push_back({
          .opcode = GetUnaryOpcode(instruction.opcode, type),
          .unary =
              {
                  .destination = ToRegister(instruction.unary.destination),
                  .operand = ToRegister(instruction.unary.operand),
              },
      });

      break;
    }

    case IrOpcode::kMultiply:
    case IrOpcode::kDivide:
    case IrOpcode::kAdd:
    case IrOpcode::kSubtract: {
      const Type type{
          GetIrValue(procedure, instruction.binary.destination).type,
      };
      FELL_ASSERT(GetIrValue(procedure, instruction.binary.left).type == type);
      FELL_ASSERT(GetIrValue(procedure, instruction.binary.right).type == type);

      module.instructions.push_back({
          .opcode = GetBinaryOpcode(instruction.opcode, type),
          .binary =
              {
                  .destination = ToRegister(instruction.binary.destination),
                  .left = ToRegister(instruction.binary.left),
                  .right = ToRegister(instruction.binary.right),
              },
      });

      break;
    }

    case IrOpcode::kEqual:
    case IrOpcode::kNotEqual:
    case IrOpcode::kLess:
    case IrOpcode::kLessEqual:
    case IrOpcode::kGreater:
    case IrOpcode::kGreaterEqual: {
      const Type operand_type{
          GetIrValue(procedure, instruction.binary.left).type,
      };
      FELL_ASSERT(GetIrValue(procedure, instruction.binary.right).type ==
                  operand_type);
      FELL_ASSERT(GetIrValue(procedure, instruction.binary.destination).type ==
                  Type::kBool);

      module.instructions.push_back({
          .opcode = GetComparisonOpcode(instruction.opcode, operand_type),
          .binary =
              {
                  .destination = ToRegister(instruction.binary.destination),
                  .left = ToRegister(instruction.binary.left),
                  .right = ToRegister(instruction.binary.right),
              },
      });

      break;
    }
  }
}

void BytecodeCompiler::CompileTerminator(const IrProcedure& procedure,
                                         const IrTerminator& terminator,
                                         const ProcedureLayout& layout,
                                         BytecodeModule& module) {
  switch (terminator.kind) {
    case IrTerminatorKind::kInvalid:
      FELL_UNREACHABLE();

    case IrTerminatorKind::kJump:
      module.instructions.push_back({
          .opcode = Opcode::kJump,
          .jump =
              {
                  .target = layout.block_targets[terminator.jump.target],
              },
      });

      break;

    case IrTerminatorKind::kBranch:
      module.instructions.push_back({
          .opcode = Opcode::kJumpIfFalse,
          .jump_if_false =
              {
                  .condition = ToRegister(terminator.branch.condition),
                  .target =
                      layout.block_targets[terminator.branch.false_target],
              },
      });

      module.instructions.push_back({
          .opcode = Opcode::kJump,
          .jump =
              {
                  .target = layout.block_targets[terminator.branch.true_target],
              },
      });

      break;

    case IrTerminatorKind::kReturn: {
      const Type type{GetIrValue(procedure, terminator.return_.value).type};

      module.instructions.push_back({
          .opcode = Opcode::kReturn,
          .return_ =
              {
                  .source = ToRegister(terminator.return_.value),
                  .type = ToValueType(type),
              },
      });

      break;
    }

    case IrTerminatorKind::kExit:
      module.instructions.push_back({
          .opcode = Opcode::kJump,
          .jump =
              {
                  .target = layout.end,
              },
      });

      break;
  }
}

BytecodeModule BytecodeCompiler::Compile(const IrProgram& ir) {
  BytecodeModule module{
      .instructions = {},
      .string_constants = ir.string_constants,
      .call_arguments = {},
      .functions = {},
      .main_instruction_count = 0,
      .register_count = GetRegisterCount(ir),
      .global_count = ir.global_count,
      .local_count = GetLocalCount(ir),
  };

  const ProcedureLayout main_layout{CompileProcedure(ir.main, module)};
  FELL_ASSERT(ir.main.entry < main_layout.block_targets.size());

  module.main_instruction_count = static_cast<u32>(module.instructions.size());

  for (const IrFunction& function : ir.functions) {
    const ProcedureLayout layout{CompileProcedure(function.procedure, module)};
    FELL_ASSERT(function.procedure.entry < layout.block_targets.size());

    Vector<LocalId> parameter_slots;

    for (IrLocalId slot : function.parameter_local_slots) {
      parameter_slots.push_back(slot);
    }

    module.functions.push_back({
        .entry = layout.block_targets[function.procedure.entry],
        .parameter_local_slots = std::move(parameter_slots),
        .return_type = ToValueType(function.return_type),
    });
  }

  return module;
}

}  // namespace fell

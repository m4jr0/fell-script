#include "compiler/bytecode_compiler.h"

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

Value MakeConstantValue(const IrProgram& ir, const IrConstant& constant) {
  const Type type{GetIrValue(ir, constant.destination).type};

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

}  // namespace

BytecodeModule BytecodeCompiler::Compile(const IrProgram& ir) {
  FELL_ASSERT(ir.values.size() <= kMaxRegisterCount);

  BytecodeModule module{
      .instructions = {},
      .string_constants = ir.string_constants,
      .call_arguments = {},
      .functions = {},
      .main_instruction_count = 0,
      .register_count = static_cast<u16>(ir.values.size()),
      .global_count = ir.global_count,
      .local_count = ir.local_count,
  };

  Vector<u32> label_targets;
  u32 bytecode_index{0};

  for (const IrInstruction& instruction : ir.instructions) {
    if (instruction.opcode == IrOpcode::kLabel) {
      if (label_targets.size() <= instruction.label.label)
        label_targets.resize(instruction.label.label + 1);
      label_targets[instruction.label.label] = bytecode_index;
    } else {
      ++bytecode_index;
    }
  }

  for (const IrValueId argument : ir.call_arguments) {
    module.call_arguments.push_back(ToRegister(argument));
  }

  for (const IrFunction& function : ir.functions) {
    FELL_ASSERT(function.entry < label_targets.size());
    Vector<LocalId> parameter_slots;

    for (IrLocalId slot : function.parameter_local_slots) {
      parameter_slots.push_back(slot);
    }

    module.functions.push_back({
        .entry = label_targets[function.entry],
        .parameter_local_slots = std::move(parameter_slots),
        .return_type = ToValueType(function.return_type),
    });
  }

  for (u32 index{0}; index < ir.main_instruction_count; ++index) {
    if (ir.instructions[index].opcode != IrOpcode::kLabel) {
      ++module.main_instruction_count;
    }
  }

  for (const IrInstruction& instruction : ir.instructions) {
    switch (instruction.opcode) {
      case IrOpcode::kInvalid:
        FELL_UNREACHABLE();

      case IrOpcode::kConstant: {
        const Type type{GetIrValue(ir, instruction.constant.destination).type};

        if (type == Type::kString) {
          module.instructions.push_back({
              .opcode = Opcode::kLoadString,
              .load_string =
                  {
                      .destination =
                          ToRegister(instruction.constant.destination),
                      .constant = instruction.constant.string_value,
                  },
          });
        } else {
          module.instructions.push_back({
              .opcode = Opcode::kLoadImmediate,
              .load_immediate =
                  {
                      .destination =
                          ToRegister(instruction.constant.destination),
                      .value = MakeConstantValue(ir, instruction.constant),
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

      case IrOpcode::kLabel:
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

      case IrOpcode::kJump:
        FELL_ASSERT(instruction.jump.target < label_targets.size());
        module.instructions.push_back({
            .opcode = Opcode::kJump,
            .jump =
                {
                    .target = label_targets[instruction.jump.target],
                },
        });
        break;

      case IrOpcode::kJumpIfFalse:
        FELL_ASSERT(instruction.jump_if_false.target < label_targets.size());
        module.instructions.push_back({
            .opcode = Opcode::kJumpIfFalse,
            .jump_if_false =
                {
                    .condition =
                        ToRegister(instruction.jump_if_false.condition),
                    .target = label_targets[instruction.jump_if_false.target],
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
                    .argument_offset = instruction.call.argument_offset,
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
            GetIrValue(ir, instruction.convert.source).type,
        };
        const Type destination_type{
            GetIrValue(ir, instruction.convert.destination).type,
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
            GetIrValue(ir, instruction.unary.destination).type,
        };

        FELL_ASSERT(GetIrValue(ir, instruction.unary.operand).type == type);

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
            GetIrValue(ir, instruction.binary.destination).type,
        };

        FELL_ASSERT(GetIrValue(ir, instruction.binary.left).type == type);
        FELL_ASSERT(GetIrValue(ir, instruction.binary.right).type == type);

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
            GetIrValue(ir, instruction.binary.left).type,
        };
        FELL_ASSERT(GetIrValue(ir, instruction.binary.right).type ==
                    operand_type);
        FELL_ASSERT(GetIrValue(ir, instruction.binary.destination).type ==
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

      case IrOpcode::kReturn: {
        const Type type{
            GetIrValue(ir, instruction.return_.value).type,
        };

        module.instructions.push_back({
            .opcode = Opcode::kReturn,
            .return_ =
                {
                    .source = ToRegister(instruction.return_.value),
                    .type = ToValueType(type),
                },
        });
        break;
      }
    }
  }

  return module;
}

}  // namespace fell
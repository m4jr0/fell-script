#include "compiler/bytecode_compiler.h"

#include "core/assert.h"
#include "core/core.h"

namespace fell {
namespace {

RegisterId ToRegister(IrValueId value) {
  FELL_ASSERT(value.value < kMaxRegisterCount);
  return static_cast<RegisterId>(value.value);
}

ValueType ToValueType(Type type) {
  switch (type) {
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

    case Type::kS64:
    case Type::kU64:
    case Type::kF64:
    case Type::kInvalid:
    case Type::kError:
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

    case Type::kInvalid:
    case Type::kError:
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

    case Type::kInvalid:
    case Type::kError:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

Value MakeConstantValue(const IrProgram& ir, const IrConstant& constant) {
  const Type type{GetIrValue(ir, constant.destination).type};

  Value value{
      .type = ToValueType(type),
      .data = {},
  };

  switch (type) {
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

    case Type::kInvalid:
    case Type::kError:
      FELL_UNREACHABLE();
  }

  return value;
}

}  // namespace

BytecodeModule BytecodeCompiler::Compile(const IrProgram& ir) {
  FELL_ASSERT(ir.values.size() <= kMaxRegisterCount);

  BytecodeModule module{
      .register_count = static_cast<u16>(ir.values.size()),
  };

  for (const IrInstruction& instruction : ir.instructions) {
    switch (instruction.opcode) {
      case IrOpcode::kConstant:
        module.instructions.push_back({
            .opcode = Opcode::kLoadImmediate,
            .load_immediate =
                {
                    .destination = ToRegister(instruction.constant.destination),
                    .value = MakeConstantValue(ir, instruction.constant),
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

      case IrOpcode::kAdd:
      case IrOpcode::kSubtract: {
        const Type type{
            GetIrValue(ir, instruction.binary.destination).type,
        };

        FELL_ASSERT(GetIrValue(ir, instruction.binary.left).type == type);
        FELL_ASSERT(GetIrValue(ir, instruction.binary.right).type == type);

        module.instructions.push_back({
            .opcode = instruction.opcode == IrOpcode::kAdd
                          ? GetAddOpcode(type)
                          : GetSubtractOpcode(type),
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
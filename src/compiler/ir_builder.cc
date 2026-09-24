#include "compiler/ir_builder.h"

#include "core/assert.h"
#include "core/core.h"

namespace fell {

IrProgram IrBuilder::Build(const CompilationUnit& unit,
                           const SemanticModel& semantics) {
  IrProgram program{};

  IrValueId last_value{};
  bool has_value{false};

  for (const Statement* statement : unit.statements) {
    switch (statement->kind) {
      case StatementKind::kExpression:
        last_value = BuildExpression(*statement->expression.expression,
                                     semantics, program);
        has_value = true;
        break;
    }
  }

  if (has_value) {
    program.instructions.push_back({
        .opcode = IrOpcode::kReturn,
        .return_ =
            {
                .value = last_value,
            },
    });
  }

  return program;
}

IrValueId IrBuilder::BuildExpression(const Expression& expression,
                                     const SemanticModel& semantics,
                                     IrProgram& program) {
  switch (expression.kind) {
    case ExpressionKind::kIntegerLiteral: {
      const Type type{semantics.Get(expression).type};
      FELL_ASSERT(type != Type::kError);

      const IrValueId destination{
          AllocateValue(program, type),
      };

      EmitIntegerConstant(program, destination,
                          expression.integer_literal.value, type);

      return destination;
    }

    case ExpressionKind::kFloatLiteral: {
      const Type type{semantics.Get(expression).type};
      FELL_ASSERT(type != Type::kError);

      const IrValueId destination{
          AllocateValue(program, type),
      };

      EmitFloatConstant(program, destination, expression.float_literal.value,
                        type);

      return destination;
    }

    case ExpressionKind::kBinary: {
      const auto& binary{expression.binary};

      IrValueId left{
          BuildExpression(*binary.left, semantics, program),
      };

      IrValueId right{
          BuildExpression(*binary.right, semantics, program),
      };

      const Type result_type{semantics.Get(expression).type};
      FELL_ASSERT(result_type != Type::kError);

      left = ConvertIfNeeded(left, result_type, program);
      right = ConvertIfNeeded(right, result_type, program);

      const IrValueId destination{
          AllocateValue(program, result_type),
      };

      IrOpcode opcode{};

      switch (binary.op) {
        case BinaryOperator::kAdd:
          opcode = IrOpcode::kAdd;
          break;

        case BinaryOperator::kSubtract:
          opcode = IrOpcode::kSubtract;
          break;
      }

      program.instructions.push_back({
          .opcode = opcode,
          .binary =
              {
                  .destination = destination,
                  .left = left,
                  .right = right,
              },
      });

      return destination;
    }
  }

  FELL_UNREACHABLE();
}

IrValueId IrBuilder::AllocateValue(IrProgram& program, Type type) {
  FELL_ASSERT(program.values.size() <= kMaxValue<u32>);

  const IrValueId id{
      .value = static_cast<u32>(program.values.size()),
  };

  program.values.push_back({
      .type = type,
  });

  return id;
}

IrValueId IrBuilder::ConvertIfNeeded(IrValueId source, Type destination_type,
                                     IrProgram& program) {
  const Type source_type{
      GetIrValue(program, source).type,
  };

  if (source_type == destination_type) {
    return source;
  }

  FELL_ASSERT(CanImplicitlyConvert(source_type, destination_type));

  const IrValueId destination{
      AllocateValue(program, destination_type),
  };

  program.instructions.push_back({
      .opcode = IrOpcode::kConvert,
      .convert =
          {
              .destination = destination,
              .source = source,
          },
  });

  return destination;
}

void IrBuilder::EmitIntegerConstant(IrProgram& program, IrValueId destination,
                                    u64 value, Type type) {
  IrConstant constant{
      .destination = destination,
  };

  switch (type) {
    case Type::kS8:
      constant.s64_value = static_cast<s8>(value);
      break;

    case Type::kS16:
      constant.s64_value = static_cast<s16>(value);
      break;

    case Type::kS32:
      constant.s64_value = static_cast<s32>(value);
      break;

    case Type::kS64:
      constant.s64_value = static_cast<s64>(value);
      break;

    case Type::kU8:
      constant.u64_value = static_cast<u8>(value);
      break;

    case Type::kU16:
      constant.u64_value = static_cast<u16>(value);
      break;

    case Type::kU32:
      constant.u64_value = static_cast<u32>(value);
      break;

    case Type::kU64:
      constant.u64_value = value;
      break;

    case Type::kF32:
    case Type::kF64:
    case Type::kInvalid:
    case Type::kError:
      FELL_UNREACHABLE();
  }

  program.instructions.push_back({
      .opcode = IrOpcode::kConstant,
      .constant = constant,
  });
}

void IrBuilder::EmitFloatConstant(IrProgram& program, IrValueId destination,
                                  f64 value, Type type) {
  IrConstant constant{
      .destination = destination,
  };

  switch (type) {
    case Type::kF32:
      constant.f64_value = static_cast<f64>(static_cast<f32>(value));
      break;

    case Type::kF64:
      constant.f64_value = value;
      break;

    case Type::kS8:
    case Type::kS16:
    case Type::kS32:
    case Type::kS64:
    case Type::kU8:
    case Type::kU16:
    case Type::kU32:
    case Type::kU64:
    case Type::kInvalid:
    case Type::kError:
      FELL_UNREACHABLE();
  }

  program.instructions.push_back({
      .opcode = IrOpcode::kConstant,
      .constant = constant,
  });
}

}  // namespace fell

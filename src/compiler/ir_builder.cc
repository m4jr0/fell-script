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
  const auto& expression_semantics{semantics.Get(expression)};

  FELL_ASSERT(expression_semantics.type == Type::kS32);

  switch (expression.kind) {
    case ExpressionKind::kIntegerLiteral: {
      const IrValueId result{AllocateValue(program)};

      program.instructions.push_back({
          .opcode = IrOpcode::kConstantS32,
          .constant_s32 =
              {
                  .destination = result,
                  .value = expression.integer_literal.value,
              },
      });

      return result;
    }

    case ExpressionKind::kBinary: {
      const auto& binary{expression.binary};

      const IrValueId left{BuildExpression(*binary.left, semantics, program)};
      const IrValueId right{BuildExpression(*binary.right, semantics, program)};
      const IrValueId result{AllocateValue(program)};

      switch (binary.op) {
        case BinaryOperator::kAdd:
          program.instructions.push_back({
              .opcode = IrOpcode::kAddS32,
              .binary_s32 =
                  {
                      .destination = result,
                      .left = left,
                      .right = right,
                  },
          });
          break;

        case BinaryOperator::kSubtract:
          program.instructions.push_back({
              .opcode = IrOpcode::kSubtractS32,
              .binary_s32 =
                  {
                      .destination = result,
                      .left = left,
                      .right = right,
                  },
          });
          break;
      }

      return result;
    }
  }

  FELL_UNREACHABLE();
}

IrValueId IrBuilder::AllocateValue(IrProgram& program) {
  FELL_ASSERT(program.value_count < kMaxValue<u32>);

  return {
      .value = program.value_count++,
  };
}

}  // namespace fell

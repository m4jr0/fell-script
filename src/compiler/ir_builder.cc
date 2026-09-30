#include "compiler/ir_builder.h"

#include "core/assert.h"
#include "core/core.h"

namespace fell {
IrProgram IrBuilder::Build(const CompilationUnit& unit,
                           const SemanticModel& semantics,
                           bool return_last_expression) {
  IrProgram program{};
  next_label_ = 0;
  loop_stack_.clear();
  program.global_count = semantics.global_count();
  program.local_count = semantics.local_count();
  IrValueId last_value{};
  bool has_value{false};

  for (const Statement* statement : unit.statements) {
    if (statement->kind == StatementKind::kExpression) {
      last_value = BuildExpression(*statement->expression.expression, semantics,
                                   program);
      has_value =
          semantics.Get(*statement->expression.expression).type != Type::kUnit;
    } else {
      BuildStatement(*statement, semantics, program);
      has_value = false;
    }
  }

  if (return_last_expression && has_value) {
    program.instructions.push_back({
        .opcode = IrOpcode::kReturn,
        .return_ = {.value = last_value},
    });
  }

  return program;
}

void IrBuilder::BuildStatement(const Statement& statement,
                               const SemanticModel& semantics,
                               IrProgram& program) {
  switch (statement.kind) {
    case StatementKind::kExpression:
      BuildExpression(*statement.expression.expression, semantics, program);
      return;

    case StatementKind::kVariableDeclaration: {
      const auto& declaration{statement.variable_declaration};
      IrValueId value{
          BuildExpression(*declaration.initializer, semantics, program)};
      const auto& statement_semantics{semantics.Get(statement)};
      value = ConvertIfNeeded(value, statement_semantics.type, program);
      if (statement_semantics.binding.storage == VariableStorage::kGlobal) {
        program.instructions.push_back({
            .opcode = IrOpcode::kStoreGlobal,
            .global = {.value = value,
                       .global = statement_semantics.binding.slot},
        });
      } else {
        program.instructions.push_back({
            .opcode = IrOpcode::kStoreLocal,
            .local = {.value = value,
                      .local = statement_semantics.binding.slot},
        });
      }
      return;
    }

    case StatementKind::kIf: {
      const IrValueId condition{
          BuildExpression(*statement.if_.condition, semantics, program)};
      const IrLabelId else_label{AllocateLabel()};
      const IrLabelId end_label{AllocateLabel()};
      program.instructions.push_back(
          {.opcode = IrOpcode::kJumpIfFalse,
           .jump_if_false = {.condition = condition, .target = else_label}});
      BuildExpression(*statement.if_.then_block, semantics, program);
      program.instructions.push_back(
          {.opcode = IrOpcode::kJump, .jump = {.target = end_label}});
      EmitLabel(program, else_label);
      if (statement.if_.else_block != nullptr)
        BuildExpression(*statement.if_.else_block, semantics, program);
      EmitLabel(program, end_label);
      return;
    }

    case StatementKind::kWhile: {
      const IrLabelId condition_label{AllocateLabel()};
      const IrLabelId end_label{AllocateLabel()};
      EmitLabel(program, condition_label);
      const IrValueId condition{
          BuildExpression(*statement.while_.condition, semantics, program)};
      program.instructions.push_back(
          {.opcode = IrOpcode::kJumpIfFalse,
           .jump_if_false = {.condition = condition, .target = end_label}});
      loop_stack_.push_back(
          {.continue_target = condition_label, .break_target = end_label});
      BuildExpression(*statement.while_.body, semantics, program);
      loop_stack_.pop_back();
      program.instructions.push_back(
          {.opcode = IrOpcode::kJump, .jump = {.target = condition_label}});
      EmitLabel(program, end_label);
      return;
    }

    case StatementKind::kFor: {
      if (statement.for_.initializer != nullptr)
        BuildStatement(*statement.for_.initializer, semantics, program);
      const IrLabelId condition_label{AllocateLabel()};
      const IrLabelId increment_label{AllocateLabel()};
      const IrLabelId end_label{AllocateLabel()};
      EmitLabel(program, condition_label);
      if (statement.for_.condition != nullptr) {
        const IrValueId condition{
            BuildExpression(*statement.for_.condition, semantics, program)};
        program.instructions.push_back(
            {.opcode = IrOpcode::kJumpIfFalse,
             .jump_if_false = {.condition = condition, .target = end_label}});
      }
      loop_stack_.push_back(
          {.continue_target = increment_label, .break_target = end_label});
      BuildExpression(*statement.for_.body, semantics, program);
      loop_stack_.pop_back();
      EmitLabel(program, increment_label);
      if (statement.for_.increment != nullptr)
        BuildExpression(*statement.for_.increment, semantics, program);
      program.instructions.push_back(
          {.opcode = IrOpcode::kJump, .jump = {.target = condition_label}});
      EmitLabel(program, end_label);
      return;
    }

    case StatementKind::kBreak:
      FELL_ASSERT(!loop_stack_.empty());
      program.instructions.push_back({
          .opcode = IrOpcode::kJump,
          .jump = {.target = loop_stack_.back().break_target},
      });
      return;

    case StatementKind::kContinue:
      FELL_ASSERT(!loop_stack_.empty());
      program.instructions.push_back({
          .opcode = IrOpcode::kJump,
          .jump = {.target = loop_stack_.back().continue_target},
      });
      return;

    case StatementKind::kSwitch: {
      const SwitchData& data{*statement.switch_.data};
      const IrValueId switch_value{
          BuildExpression(*data.value, semantics, program)};
      const IrLabelId end_label{AllocateLabel()};
      for (const SwitchCase& case_ : data.cases) {
        const IrLabelId next_label{AllocateLabel()};
        IrValueId case_value{BuildExpression(*case_.value, semantics, program)};
        const Type switch_type{GetIrValue(program, switch_value).type};
        case_value = ConvertIfNeeded(case_value, switch_type, program);
        const IrValueId matches{AllocateValue(program, Type::kBool)};
        program.instructions.push_back({.opcode = IrOpcode::kEqual,
                                        .binary = {.destination = matches,
                                                   .left = switch_value,
                                                   .right = case_value}});
        program.instructions.push_back(
            {.opcode = IrOpcode::kJumpIfFalse,
             .jump_if_false = {.condition = matches, .target = next_label}});
        BuildExpression(*case_.body, semantics, program);
        program.instructions.push_back(
            {.opcode = IrOpcode::kJump, .jump = {.target = end_label}});
        EmitLabel(program, next_label);
      }
      if (data.default_body != nullptr)
        BuildExpression(*data.default_body, semantics, program);
      EmitLabel(program, end_label);
      return;
    }
  }
  FELL_UNREACHABLE();
}

IrValueId IrBuilder::BuildExpression(const Expression& expression,
                                     const SemanticModel& semantics,
                                     IrProgram& program) {
  switch (expression.kind) {
    case ExpressionKind::kBooleanLiteral: {
      const Type type{semantics.Get(expression).type};
      FELL_ASSERT(type != Type::kError);
      const IrValueId destination{AllocateValue(program, type)};
      EmitBooleanConstant(program, destination,
                          expression.boolean_literal.value);
      return destination;
    }

    case ExpressionKind::kIntegerLiteral: {
      const Type type{semantics.Get(expression).type};
      FELL_ASSERT(type != Type::kError);
      const IrValueId destination{AllocateValue(program, type)};
      EmitIntegerConstant(program, destination,
                          expression.integer_literal.value, type);
      return destination;
    }

    case ExpressionKind::kFloatLiteral: {
      const Type type{semantics.Get(expression).type};
      FELL_ASSERT(type != Type::kError);
      const IrValueId destination{AllocateValue(program, type)};
      EmitFloatConstant(program, destination, expression.float_literal.value,
                        type);
      return destination;
    }

    case ExpressionKind::kStringLiteral: {
      const IrValueId destination{AllocateValue(program, Type::kString)};
      EmitStringConstant(program, destination, expression.string_literal.value);
      return destination;
    }

    case ExpressionKind::kVariable: {
      const auto& expression_semantics{semantics.Get(expression)};
      const IrValueId destination{
          AllocateValue(program, expression_semantics.type)};
      if (expression_semantics.binding.storage == VariableStorage::kGlobal) {
        program.instructions.push_back({
            .opcode = IrOpcode::kLoadGlobal,
            .global = {.value = destination,
                       .global = expression_semantics.binding.slot},
        });
      } else {
        program.instructions.push_back({
            .opcode = IrOpcode::kLoadLocal,
            .local = {.value = destination,
                      .local = expression_semantics.binding.slot},
        });
      }
      return destination;
    }

    case ExpressionKind::kAssignment: {
      const auto& expression_semantics{semantics.Get(expression)};
      IrValueId value{
          BuildExpression(*expression.assignment.value, semantics, program)};
      value = ConvertIfNeeded(value, expression_semantics.type, program);
      if (expression_semantics.binding.storage == VariableStorage::kGlobal) {
        program.instructions.push_back({
            .opcode = IrOpcode::kStoreGlobal,
            .global = {.value = value,
                       .global = expression_semantics.binding.slot},
        });
      } else {
        program.instructions.push_back({
            .opcode = IrOpcode::kStoreLocal,
            .local = {.value = value,
                      .local = expression_semantics.binding.slot},
        });
      }
      return value;
    }

    case ExpressionKind::kConditional: {
      const auto& conditional{expression.conditional};
      const Type result_type{semantics.Get(expression).type};
      const IrValueId destination{AllocateValue(program, result_type)};
      const IrValueId condition{
          BuildExpression(*conditional.condition, semantics, program)};
      const IrLabelId else_label{AllocateLabel()};
      const IrLabelId end_label{AllocateLabel()};
      program.instructions.push_back(
          {.opcode = IrOpcode::kJumpIfFalse,
           .jump_if_false = {.condition = condition, .target = else_label}});
      IrValueId then_value{
          BuildExpression(*conditional.then_expression, semantics, program)};
      then_value = ConvertIfNeeded(then_value, result_type, program);
      program.instructions.push_back(
          {.opcode = IrOpcode::kMove,
           .move = {.destination = destination, .source = then_value}});
      program.instructions.push_back(
          {.opcode = IrOpcode::kJump, .jump = {.target = end_label}});
      EmitLabel(program, else_label);
      IrValueId else_value{
          BuildExpression(*conditional.else_expression, semantics, program)};
      else_value = ConvertIfNeeded(else_value, result_type, program);
      program.instructions.push_back(
          {.opcode = IrOpcode::kMove,
           .move = {.destination = destination, .source = else_value}});
      EmitLabel(program, end_label);
      return destination;
    }

    case ExpressionKind::kBlock: {
      for (const Statement* statement : expression.block.body->statements) {
        BuildStatement(*statement, semantics, program);
      }
      if (expression.block.trailing_expression == nullptr) {
        return {};
      }
      return BuildExpression(*expression.block.trailing_expression, semantics,
                             program);
    }

    case ExpressionKind::kUnary: {
      const auto& unary{expression.unary};

      // Lower negated integer literals directly as signed constants.
      if (unary.op == UnaryOperator::kNegate &&
          unary.operand->kind == ExpressionKind::kIntegerLiteral &&
          (unary.operand->integer_literal.explicit_type == Type::kInvalid ||
           IsSignedInteger(unary.operand->integer_literal.explicit_type))) {
        const Type type{semantics.Get(expression).type};
        FELL_ASSERT(type != Type::kError);
        FELL_ASSERT(IsSignedInteger(type));

        const IrValueId destination{AllocateValue(program, type)};
        EmitNegatedIntegerConstant(program, destination,
                                   unary.operand->integer_literal.value, type);
        return destination;
      }

      const IrValueId operand{
          BuildExpression(*unary.operand, semantics, program)};

      const Type result_type{semantics.Get(expression).type};
      FELL_ASSERT(result_type != Type::kError);

      const IrValueId destination{AllocateValue(program, result_type)};
      IrOpcode opcode{};
      switch (unary.op) {
        case UnaryOperator::kNegate:
          opcode = IrOpcode::kNegate;
          break;
        case UnaryOperator::kLogicalNot:
          opcode = IrOpcode::kLogicalNot;
          break;
      }

      program.instructions.push_back({
          .opcode = opcode,
          .unary = {.destination = destination, .operand = operand},
      });

      return destination;
    }

    case ExpressionKind::kBinary: {
      const auto& binary{expression.binary};
      if (binary.op == BinaryOperator::kLogicalAnd ||
          binary.op == BinaryOperator::kLogicalOr) {
        const IrValueId destination{AllocateValue(program, Type::kBool)};
        const IrValueId left{BuildExpression(*binary.left, semantics, program)};
        const IrLabelId rhs_label{AllocateLabel()};
        const IrLabelId false_label{AllocateLabel()};
        const IrLabelId end_label{AllocateLabel()};

        if (binary.op == BinaryOperator::kLogicalAnd) {
          program.instructions.push_back(
              {.opcode = IrOpcode::kJumpIfFalse,
               .jump_if_false = {.condition = left, .target = false_label}});
          const IrValueId right{
              BuildExpression(*binary.right, semantics, program)};
          program.instructions.push_back(
              {.opcode = IrOpcode::kMove,
               .move = {.destination = destination, .source = right}});
          program.instructions.push_back(
              {.opcode = IrOpcode::kJump, .jump = {.target = end_label}});
          EmitLabel(program, false_label);
          program.instructions.push_back(
              {.opcode = IrOpcode::kMove,
               .move = {.destination = destination, .source = left}});
        } else {
          program.instructions.push_back(
              {.opcode = IrOpcode::kJumpIfFalse,
               .jump_if_false = {.condition = left, .target = rhs_label}});
          program.instructions.push_back(
              {.opcode = IrOpcode::kMove,
               .move = {.destination = destination, .source = left}});
          program.instructions.push_back(
              {.opcode = IrOpcode::kJump, .jump = {.target = end_label}});
          EmitLabel(program, rhs_label);
          const IrValueId right{
              BuildExpression(*binary.right, semantics, program)};
          program.instructions.push_back(
              {.opcode = IrOpcode::kMove,
               .move = {.destination = destination, .source = right}});
        }
        EmitLabel(program, end_label);
        return destination;
      }

      IrValueId left{BuildExpression(*binary.left, semantics, program)};
      IrValueId right{BuildExpression(*binary.right, semantics, program)};

      const auto& expression_semantics{semantics.Get(expression)};
      const Type result_type{expression_semantics.type};
      const Type operand_type{expression_semantics.operand_type ==
                                      Type::kInvalid
                                  ? result_type
                                  : expression_semantics.operand_type};
      FELL_ASSERT(result_type != Type::kError);
      left = ConvertIfNeeded(left, operand_type, program);
      right = ConvertIfNeeded(right, operand_type, program);

      const IrValueId destination{AllocateValue(program, result_type)};
      IrOpcode opcode{};
      switch (binary.op) {
        case BinaryOperator::kLogicalAnd:
        case BinaryOperator::kLogicalOr:
          FELL_UNREACHABLE();
        case BinaryOperator::kMultiply:
          opcode = IrOpcode::kMultiply;
          break;
        case BinaryOperator::kDivide:
          opcode = IrOpcode::kDivide;
          break;
        case BinaryOperator::kAdd:
          opcode = IrOpcode::kAdd;
          break;
        case BinaryOperator::kSubtract:
          opcode = IrOpcode::kSubtract;
          break;
        case BinaryOperator::kEqual:
          opcode = IrOpcode::kEqual;
          break;
        case BinaryOperator::kNotEqual:
          opcode = IrOpcode::kNotEqual;
          break;
        case BinaryOperator::kLess:
          opcode = IrOpcode::kLess;
          break;
        case BinaryOperator::kLessEqual:
          opcode = IrOpcode::kLessEqual;
          break;
        case BinaryOperator::kGreater:
          opcode = IrOpcode::kGreater;
          break;
        case BinaryOperator::kGreaterEqual:
          opcode = IrOpcode::kGreaterEqual;
          break;
      }

      program.instructions.push_back({
          .opcode = opcode,
          .binary = {.destination = destination, .left = left, .right = right},
      });

      return destination;
    }
  }

  FELL_UNREACHABLE();
}

IrLabelId IrBuilder::AllocateLabel() {
  FELL_ASSERT(next_label_ < kMaxValue<IrLabelId>);
  return next_label_++;
}

void IrBuilder::EmitLabel(IrProgram& program, IrLabelId label) {
  program.instructions.push_back(
      {.opcode = IrOpcode::kLabel, .label = {.label = label}});
}

IrValueId IrBuilder::AllocateValue(IrProgram& program, Type type) {
  FELL_ASSERT(program.values.size() < kMaxValue<u32>);
  const IrValueId id{.value = static_cast<u32>(program.values.size())};
  program.values.push_back({.type = type});
  return id;
}

IrValueId IrBuilder::ConvertIfNeeded(IrValueId source, Type destination_type,
                                     IrProgram& program) {
  const Type source_type{GetIrValue(program, source).type};
  if (source_type == destination_type) {
    return source;
  }

  FELL_ASSERT(CanImplicitlyConvert(source_type, destination_type));
  const IrValueId destination{AllocateValue(program, destination_type)};
  program.instructions.push_back({
      .opcode = IrOpcode::kConvert,
      .convert = {.destination = destination, .source = source},
  });

  return destination;
}
void IrBuilder::EmitBooleanConstant(IrProgram& program, IrValueId destination,
                                    bool value) {
  IrConstant constant{.destination = destination, .bool_value = value};

  program.instructions.push_back({
      .opcode = IrOpcode::kConstant,
      .constant = constant,
  });
}

void IrBuilder::EmitIntegerConstant(IrProgram& program, IrValueId destination,
                                    u64 value, Type type) {
  IrConstant constant{.destination = destination, .s64_value = 0};

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
    case Type::kBool:
    case Type::kString:
    case Type::kF32:
    case Type::kF64:
    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
      FELL_UNREACHABLE();
  }

  program.instructions.push_back({
      .opcode = IrOpcode::kConstant,
      .constant = constant,
  });
}

void IrBuilder::EmitNegatedIntegerConstant(IrProgram& program,
                                           IrValueId destination, u64 magnitude,
                                           Type type) {
  FELL_ASSERT(IsSignedInteger(type));
  FELL_ASSERT(CanRepresentNegativeInteger(type, magnitude));

  IrConstant constant{.destination = destination, .s64_value = 0};

  if (magnitude == static_cast<u64>(kMaxValue<s64>) + 1) {
    constant.s64_value = kMinValue<s64>;
  } else {
    constant.s64_value = -static_cast<s64>(magnitude);
  }

  program.instructions.push_back({
      .opcode = IrOpcode::kConstant,
      .constant = constant,
  });
}

void IrBuilder::EmitFloatConstant(IrProgram& program, IrValueId destination,
                                  f64 value, Type type) {
  IrConstant constant{.destination = destination, .s64_value = 0};

  switch (type) {
    case Type::kF32:
      constant.f64_value = static_cast<f64>(static_cast<f32>(value));
      break;
    case Type::kF64:
      constant.f64_value = value;
      break;
    case Type::kBool:
    case Type::kString:
    case Type::kS8:
    case Type::kS16:
    case Type::kS32:
    case Type::kS64:
    case Type::kU8:
    case Type::kU16:
    case Type::kU32:
    case Type::kU64:
    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
      FELL_UNREACHABLE();
  }

  program.instructions.push_back({
      .opcode = IrOpcode::kConstant,
      .constant = constant,
  });
}

void IrBuilder::EmitStringConstant(IrProgram& program, IrValueId destination,
                                   StringView value) {
  FELL_ASSERT(program.string_constants.size() < kMaxValue<u32>);
  const StringConstantId id{static_cast<u32>(program.string_constants.size())};
  program.string_constants.emplace_back(value);
  program.instructions.push_back({
      .opcode = IrOpcode::kConstant,
      .constant = {.destination = destination, .string_value = id},
  });
}

}  // namespace fell

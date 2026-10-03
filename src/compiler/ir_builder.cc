#include "compiler/ir_builder.h"

#include <utility>

#include "core/assert.h"
#include "core/type.h"

namespace fell {

IrProgram IrBuilder::Build(const CompilationUnit& unit,
                           const SemanticModel& semantics,
                           bool return_last_expression) {
  IrProgram program{};

  program.global_count = semantics.global_count();

  program.main.entry = CreateBlock(program.main);
  program.main.local_count = semantics.local_count();

  SetCurrentBlock(program.main.entry);

  IrValueId last_value{};
  bool has_value{false};

  for (const Statement* statement : unit.statements) {
    if (statement->kind == StatementKind::kFunctionDeclaration) {
      continue;
    }

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
    EmitInstruction(procedure, {
                                   .opcode = IrOpcode::kReturn,
                                   .return_ =
                                       {
                                           .value = last_value,
                                       },
                               });
  }

  program.main_instruction_count =
      static_cast<u32>(program.instructions.size());

  for (const FunctionSemantics& function : semantics.functions()) {
    const IrLabelId entry{AllocateLabel()};

    Vector<IrLocalId> parameter_slots;

    for (u32 slot : function.parameter_local_slots) {
      parameter_slots.push_back(slot);
    }

    program.functions.push_back({
        .id = static_cast<IrFunctionId>(program.functions.size()),
        .entry = entry,
        .parameter_local_slots = std::move(parameter_slots),
        .return_type = function.return_type,
    });

    EmitLabel(program, entry);

    BuildExpression(*function.declaration->function_declaration.data->body,
                    semantics, program);
  }

  return program;
}

void IrBuilder::BuildStatement(const Statement& statement,
                               const SemanticModel& semantics,
                               IrProgram& program, IrProcedure& procedure) {
  switch (statement.kind) {
    case StatementKind::kInvalid:
      FELL_UNREACHABLE();

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
        EmitInstruction(procedure,
                        {
                            .opcode = IrOpcode::kStoreGlobal,
                            .global =
                                {
                                    .value = value,
                                    .global = statement_semantics.binding.slot,
                                },
                        });
      } else {
        EmitInstruction(procedure,
                        {
                            .opcode = IrOpcode::kStoreLocal,
                            .local =
                                {
                                    .value = value,
                                    .local = statement_semantics.binding.slot,
                                },
                        });
      }

      return;
    }

    case StatementKind::kFunctionDeclaration:
      return;

    case StatementKind::kReturn: {
      IrValueId value{
          BuildExpression(*statement.return_.value, semantics, program)};

      value = ConvertIfNeeded(value, semantics.Get(statement).type, program);

      EmitInstruction(procedure, {
                                     .opcode = IrOpcode::kReturn,
                                     .return_ =
                                         {
                                             .value = value,
                                         },
                                 });

      return;
    }

    case StatementKind::kIf: {
      const IrValueId condition{BuildExpression(*statement.if_.condition,
                                                semantics, program, procedure)};

      const IrBlockId then_block{CreateBlock(procedure)};
      const IrBlockId else_block{CreateBlock(procedure)};
      const IrBlockId end_block{CreateBlock(procedure)};

      EmitBranch(procedure, condition, then_block, else_block);
      SetCurrentBlock(then_block);
      BuildExpression(*statement.if_.then_block, semantics, program, procedure);
      EmitJump(procedure, end_block);
      SetCurrentBlock(else_block);

      if (statement.if_.else_block != nullptr) {
        BuildExpression(*statement.if_.else_block, semantics, program,
                        procedure);
      }

      if (!IsBlockTerminated(GetCurrentBlock(procedure))) {
        EmitJump(procedure, end_block);
      }

      SetCurrentBlock(end_block);
      return;
    }

    case StatementKind::kWhile: {
      const IrBlockId condition_block{CreateBlock(procedure)};
      const IrBlockId body_block{CreateBlock(procedure)};
      const IrBlockId end_block{CreateBlock(procedure)};

      EmitJump(procedure, condition_block);
      SetCurrentBlock(condition_block);

      const IrValueId condition{BuildExpression(*statement.while_.condition,
                                                semantics, program, procedure)};
      EmitBranch(procedure, condition, body_block, end_block);

      SetCurrentBlock(body_block);

      loop_stack_.push_back({
          .continue_target = condition_block,
          .break_target = end_block,
      });

      BuildExpression(...);

      loop_stack_.pop_back();

      if (!IsBlockTerminated(GetCurrentBlock(procedure))) {
        EmitJump(procedure, condition_block);
      }

      SetCurrentBlock(end_block);
      return;
    }

    case StatementKind::kFor: {
      if (statement.for_.initializer != nullptr) {
        BuildStatement(*statement.for_.initializer, semantics, program);
      }

      const IrLabelId condition_label{AllocateLabel()};
      const IrLabelId increment_label{AllocateLabel()};
      const IrLabelId end_label{AllocateLabel()};

      EmitLabel(program, condition_label);

      if (statement.for_.condition != nullptr) {
        const IrValueId condition{
            BuildExpression(*statement.for_.condition, semantics, program)};

        EmitInstruction(procedure, {
                                       .opcode = IrOpcode::kJumpIfFalse,
                                       .jump_if_false =
                                           {
                                               .condition = condition,
                                               .target = end_label,
                                           },
                                   });
      }

      loop_stack_.push_back({
          .continue_target = increment_label,
          .break_target = end_label,
      });

      BuildExpression(*statement.for_.body, semantics, program);

      loop_stack_.pop_back();

      EmitLabel(program, increment_label);

      if (statement.for_.increment != nullptr) {
        BuildExpression(*statement.for_.increment, semantics, program);
      }

      EmitInstruction(procedure, {
                                     .opcode = IrOpcode::kJump,
                                     .jump =
                                         {
                                             .target = condition_label,
                                         },
                                 });

      EmitLabel(program, end_label);

      return;
    }

    case StatementKind::kBreak:
      FELL_ASSERT(!loop_stack_.empty());

      EmitInstruction(procedure,
                      {
                          .opcode = IrOpcode::kJump,
                          .jump =
                              {
                                  .target = loop_stack_.back().break_target,
                              },
                      });

      return;

    case StatementKind::kContinue:
      FELL_ASSERT(!loop_stack_.empty());

      EmitInstruction(procedure,
                      {
                          .opcode = IrOpcode::kJump,
                          .jump =
                              {
                                  .target = loop_stack_.back().continue_target,
                              },
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

        EmitInstruction(procedure, {
                                       .opcode = IrOpcode::kEqual,
                                       .binary =
                                           {
                                               .destination = matches,
                                               .left = switch_value,
                                               .right = case_value,
                                           },
                                   });

        EmitInstruction(procedure, {
                                       .opcode = IrOpcode::kJumpIfFalse,
                                       .jump_if_false =
                                           {
                                               .condition = matches,
                                               .target = next_label,
                                           },
                                   });

        BuildExpression(*case_.body, semantics, program);

        EmitInstruction(procedure, {
                                       .opcode = IrOpcode::kJump,
                                       .jump =
                                           {
                                               .target = end_label,
                                           },
                                   });

        EmitLabel(program, next_label);
      }

      if (data.default_body != nullptr) {
        BuildExpression(*data.default_body, semantics, program);
      }

      EmitLabel(program, end_label);

      return;
    }
  }

  FELL_UNREACHABLE();
}

IrValueId IrBuilder::BuildExpression(const Expression& expression,
                                     const SemanticModel& semantics,
                                     IrProgram& program,
                                     IrProcedure& procedure) {
  switch (expression.kind) {
    case ExpressionKind::kInvalid:
      FELL_UNREACHABLE();

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
        EmitInstruction(procedure,
                        {
                            .opcode = IrOpcode::kLoadGlobal,
                            .global =
                                {
                                    .value = destination,
                                    .global = expression_semantics.binding.slot,
                                },
                        });
      } else {
        EmitInstruction(procedure,
                        {
                            .opcode = IrOpcode::kLoadLocal,
                            .local =
                                {
                                    .value = destination,
                                    .local = expression_semantics.binding.slot,
                                },
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
        EmitInstruction(procedure,
                        {
                            .opcode = IrOpcode::kStoreGlobal,
                            .global =
                                {
                                    .value = value,
                                    .global = expression_semantics.binding.slot,
                                },
                        });
      } else {
        EmitInstruction(procedure,
                        {
                            .opcode = IrOpcode::kStoreLocal,
                            .local =
                                {
                                    .value = value,
                                    .local = expression_semantics.binding.slot,
                                },
                        });
      }

      return value;
    }

    case ExpressionKind::kCall: {
      const CallData& call{*expression.call.data};
      const ExpressionSemantics& call_semantics{semantics.Get(expression)};

      switch (call_semantics.function.storage) {
        case FunctionStorage::kInvalid:
          FELL_UNREACHABLE();

        case FunctionStorage::kNative: {
          FELL_ASSERT(call.arguments.size() == 1);

          const IrValueId argument{
              BuildExpression(*call.arguments[0], semantics, program)};

          EmitInstruction(
              procedure, {
                             .opcode = IrOpcode::kCallNative,
                             .call_native =
                                 {
                                     .argument = argument,
                                     .argument_type =
                                         semantics.Get(*call.arguments[0]).type,
                                     .function = call_semantics.function.slot,
                                 },
                         });

          return {};
        }

        case FunctionStorage::kFell: {
          FELL_ASSERT(call_semantics.function.slot <
                      semantics.functions().size());

          const FunctionSemantics& function{
              semantics.functions()[call_semantics.function.slot]};

          const u32 argument_offset{
              static_cast<u32>(program.call_arguments.size())};

          for (usize index{0}; index < call.arguments.size(); ++index) {
            IrValueId argument{
                BuildExpression(*call.arguments[index], semantics, program)};

            argument = ConvertIfNeeded(
                argument, function.parameter_types[index], program);

            program.call_arguments.push_back(argument);
          }

          const IrValueId destination{
              AllocateValue(program, call_semantics.type)};

          EmitInstruction(procedure,
                          {
                              .opcode = IrOpcode::kCall,
                              .call =
                                  {
                                      .destination = destination,
                                      .function = call_semantics.function.slot,
                                      .argument_offset = argument_offset,
                                      .argument_count = static_cast<u32>(
                                          call.arguments.size()),
                                      .has_destination = true,
                                  },
                          });

          return destination;
        }
      }

      FELL_UNREACHABLE();
    }

    case ExpressionKind::kConditional: {
      const auto& conditional{expression.conditional};

      const Type result_type{semantics.Get(expression).type};
      const IrValueId destination{AllocateValue(program, result_type)};

      const IrValueId condition{
          BuildExpression(*conditional.condition, semantics, program)};

      const IrLabelId else_label{AllocateLabel()};
      const IrLabelId end_label{AllocateLabel()};

      EmitInstruction(procedure, {
                                     .opcode = IrOpcode::kJumpIfFalse,
                                     .jump_if_false =
                                         {
                                             .condition = condition,
                                             .target = else_label,
                                         },
                                 });

      IrValueId then_value{
          BuildExpression(*conditional.then_expression, semantics, program)};

      then_value = ConvertIfNeeded(then_value, result_type, program);

      EmitInstruction(procedure, {
                                     .opcode = IrOpcode::kMove,
                                     .move =
                                         {
                                             .destination = destination,
                                             .source = then_value,
                                         },
                                 });

      EmitInstruction(procedure, {
                                     .opcode = IrOpcode::kJump,
                                     .jump =
                                         {
                                             .target = end_label,
                                         },
                                 });

      EmitLabel(program, else_label);

      IrValueId else_value{
          BuildExpression(*conditional.else_expression, semantics, program)};

      else_value = ConvertIfNeeded(else_value, result_type, program);

      EmitInstruction(procedure, {
                                     .opcode = IrOpcode::kMove,
                                     .move =
                                         {
                                             .destination = destination,
                                             .source = else_value,
                                         },
                                 });

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
        case UnaryOperator::kInvalid:
          FELL_UNREACHABLE();

        case UnaryOperator::kNegate:
          opcode = IrOpcode::kNegate;
          break;

        case UnaryOperator::kLogicalNot:
          opcode = IrOpcode::kLogicalNot;
          break;
      }

      EmitInstruction(procedure, {
                                     .opcode = opcode,
                                     .unary =
                                         {
                                             .destination = destination,
                                             .operand = operand,
                                         },
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
          EmitInstruction(procedure, {
                                         .opcode = IrOpcode::kJumpIfFalse,
                                         .jump_if_false =
                                             {
                                                 .condition = left,
                                                 .target = false_label,
                                             },
                                     });

          const IrValueId right{
              BuildExpression(*binary.right, semantics, program)};

          EmitInstruction(procedure, {
                                         .opcode = IrOpcode::kMove,
                                         .move =
                                             {
                                                 .destination = destination,
                                                 .source = right,
                                             },
                                     });

          EmitInstruction(procedure, {
                                         .opcode = IrOpcode::kJump,
                                         .jump =
                                             {
                                                 .target = end_label,
                                             },
                                     });

          EmitLabel(program, false_label);

          EmitInstruction(procedure, {
                                         .opcode = IrOpcode::kMove,
                                         .move =
                                             {
                                                 .destination = destination,
                                                 .source = left,
                                             },
                                     });
        } else {
          EmitInstruction(procedure, {
                                         .opcode = IrOpcode::kJumpIfFalse,
                                         .jump_if_false =
                                             {
                                                 .condition = left,
                                                 .target = rhs_label,
                                             },
                                     });

          EmitInstruction(procedure, {
                                         .opcode = IrOpcode::kMove,
                                         .move =
                                             {
                                                 .destination = destination,
                                                 .source = left,
                                             },
                                     });

          EmitInstruction(procedure, {
                                         .opcode = IrOpcode::kJump,
                                         .jump =
                                             {
                                                 .target = end_label,
                                             },
                                     });

          EmitLabel(program, rhs_label);

          const IrValueId right{
              BuildExpression(*binary.right, semantics, program)};

          EmitInstruction(procedure, {
                                         .opcode = IrOpcode::kMove,
                                         .move =
                                             {
                                                 .destination = destination,
                                                 .source = right,
                                             },
                                     });
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
        case BinaryOperator::kInvalid:
          FELL_UNREACHABLE();

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

      EmitInstruction(procedure, {
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

IrValueId IrBuilder::AllocateValue(IrProcedure& procedure, Type type);
{
  FELL_ASSERT(program.values.size() < kMaxValue<u32>);

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
  const Type source_type{GetIrValue(program, source).type};

  if (source_type == destination_type) {
    return source;
  }

  FELL_ASSERT(CanImplicitlyConvert(source_type, destination_type));

  const IrValueId destination{AllocateValue(program, destination_type)};

  EmitInstruction(procedure, {
                                 .opcode = IrOpcode::kConvert,
                                 .convert =
                                     {
                                         .destination = destination,
                                         .source = source,
                                     },
                             });

  return destination;
}
IrBlockId IrBuilder::CreateBlock(IrProcedure& procedure) {
  const IrBlockId id{static_cast<IrBlockId>(procedure.blocks.size())};
  procedure.blocks.emplace_back();
  return id;
}

bool IrBuilder::IsBlockTerminated(const IrBasicBlock& block) {
  return block.terminator.kind != IrTerminatorKind::kInvalid;
}

void IrBuilder::EmitBooleanConstant(IrProgram& program, IrValueId destination,
                                    bool value) {
  IrConstant constant{
      .destination = destination,
      .bool_value = value,
  };

  EmitInstruction(procedure, {
                                 .opcode = IrOpcode::kConstant,
                                 .constant = constant,
                             });
}

void IrBuilder::EmitIntegerConstant(IrProgram& program, IrValueId destination,
                                    u64 value, Type type) {
  IrConstant constant{
      .destination = destination,
      .s64_value = 0,
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

    case Type::kBool:
    case Type::kString:
    case Type::kF32:
    case Type::kF64:
    case Type::kUnit:
    case Type::kInvalid:
    case Type::kError:
      FELL_UNREACHABLE();
  }

  EmitInstruction(procedure, {
                                 .opcode = IrOpcode::kConstant,
                                 .constant = constant,
                             });
}

void IrBuilder::EmitNegatedIntegerConstant(IrProgram& program,
                                           IrValueId destination, u64 magnitude,
                                           Type type) {
  FELL_ASSERT(IsSignedInteger(type));
  FELL_ASSERT(CanRepresentNegativeInteger(type, magnitude));

  IrConstant constant{
      .destination = destination,
      .s64_value = 0,
  };

  if (magnitude == static_cast<u64>(kMaxValue<s64>) + 1) {
    constant.s64_value = kMinValue<s64>;
  } else {
    constant.s64_value = -static_cast<s64>(magnitude);
  }

  EmitInstruction(procedure, {
                                 .opcode = IrOpcode::kConstant,
                                 .constant = constant,
                             });
}

void IrBuilder::EmitFloatConstant(IrProgram& program, IrValueId destination,
                                  f64 value, Type type) {
  IrConstant constant{
      .destination = destination,
      .s64_value = 0,
  };

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

  EmitInstruction(procedure, {
                                 .opcode = IrOpcode::kConstant,
                                 .constant = constant,
                             });
}

void IrBuilder::EmitStringConstant(IrProgram& program, IrValueId destination,
                                   StringView value) {
  FELL_ASSERT(program.string_constants.size() < kMaxValue<u32>);

  const IrStringConstantId id{
      static_cast<u32>(program.string_constants.size())};

  program.string_constants.emplace_back(value);

  EmitInstruction(procedure, {
                                 .opcode = IrOpcode::kConstant,
                                 .constant =
                                     {
                                         .destination = destination,
                                         .string_value = id,
                                     },
                             });
}

}  // namespace fell
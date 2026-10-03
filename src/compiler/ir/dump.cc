#include "compiler/ir/dump.h"

#include <sstream>

#include "compiler/frontend/lexer.h"
#include "compiler/frontend/token.h"
#include "compiler/frontend/type.h"
#include "core/assert.h"
#include "core/type.h"

namespace fell {
namespace {

StringView ToString(UnaryOperator op) {
  switch (op) {
    case UnaryOperator::kInvalid:
      FELL_UNREACHABLE();

    case UnaryOperator::kNegate:
      return "-";
    case UnaryOperator::kLogicalNot:
      return "!";
  }

  FELL_UNREACHABLE();
}

StringView ToString(BinaryOperator op) {
  switch (op) {
    case BinaryOperator::kInvalid:
      FELL_UNREACHABLE();

    case BinaryOperator::kLogicalAnd:
      return "&&";
    case BinaryOperator::kLogicalOr:
      return "||";
    case BinaryOperator::kMultiply:
      return "*";
    case BinaryOperator::kDivide:
      return "/";
    case BinaryOperator::kAdd:
      return "+";
    case BinaryOperator::kSubtract:
      return "-";
    case BinaryOperator::kEqual:
      return "==";
    case BinaryOperator::kNotEqual:
      return "!=";
    case BinaryOperator::kLess:
      return "<";
    case BinaryOperator::kLessEqual:
      return "<=";
    case BinaryOperator::kGreater:
      return ">";
    case BinaryOperator::kGreaterEqual:
      return ">=";
  }

  FELL_UNREACHABLE();
}

StringView ToString(IrOpcode opcode) {
  switch (opcode) {
    case IrOpcode::kInvalid:
      FELL_UNREACHABLE();

    case IrOpcode::kConstant:
      return "constant";
    case IrOpcode::kConvert:
      return "convert";
    case IrOpcode::kLoadGlobal:
      return "load_global";
    case IrOpcode::kStoreGlobal:
      return "store_global";
    case IrOpcode::kLoadLocal:
      return "load_local";
    case IrOpcode::kStoreLocal:
      return "store_local";
    case IrOpcode::kMove:
      return "move";
    case IrOpcode::kCall:
      return "call";
    case IrOpcode::kCallNative:
      return "call_native";
    case IrOpcode::kNegate:
      return "negate";
    case IrOpcode::kLogicalNot:
      return "logical_not";
    case IrOpcode::kMultiply:
      return "multiply";
    case IrOpcode::kDivide:
      return "divide";
    case IrOpcode::kAdd:
      return "add";
    case IrOpcode::kSubtract:
      return "subtract";
    case IrOpcode::kEqual:
      return "equal";
    case IrOpcode::kNotEqual:
      return "not_equal";
    case IrOpcode::kLess:
      return "less";
    case IrOpcode::kLessEqual:
      return "less_equal";
    case IrOpcode::kGreater:
      return "greater";
    case IrOpcode::kGreaterEqual:
      return "greater_equal";
  }

  FELL_UNREACHABLE();
}

constexpr usize kIndentSize{2};

void WriteIndent(std::ostringstream& output, usize depth) {
  output << String(depth * kIndentSize, ' ');
}

void DumpExpression(const Expression& expression, std::ostringstream& output,
                    usize depth);

void DumpStatement(const Statement& statement, std::ostringstream& output,
                   usize depth) {
  WriteIndent(output, depth);

  switch (statement.kind) {
    case StatementKind::kInvalid:
      FELL_UNREACHABLE();

    case StatementKind::kExpression:
      output << "ExpressionStatement\n";
      DumpExpression(*statement.expression.expression, output, depth + 1);
      return;

    case StatementKind::kVariableDeclaration: {
      const auto& declaration{statement.variable_declaration};
      output << "VariableDeclaration " << (declaration.is_mutable ? "mut " : "")
             << declaration.name;

      if (declaration.explicit_type != Type::kInvalid) {
        output << ": " << ToString(declaration.explicit_type);
      }

      output << '\n';
      DumpExpression(*declaration.initializer, output, depth + 1);
      return;
    }

    case StatementKind::kFunctionDeclaration: {
      const FunctionData& function{*statement.function_declaration.data};
      output << "FunctionDeclaration " << function.name << " -> "
             << ToString(function.return_type) << '\n';

      for (const FunctionParameter& parameter : function.parameters) {
        WriteIndent(output, depth + 1);
        output << "Parameter " << parameter.name << ": "
               << ToString(parameter.type) << '\n';
      }

      DumpExpression(*function.body, output, depth + 1);
      return;
    }

    case StatementKind::kReturn:
      output << "ReturnStatement\n";
      DumpExpression(*statement.return_.value, output, depth + 1);
      return;

    case StatementKind::kIf:
      output << "IfStatement\n";
      WriteIndent(output, depth + 1);
      output << "Condition\n";
      DumpExpression(*statement.if_.condition, output, depth + 2);
      WriteIndent(output, depth + 1);
      output << "Then\n";
      DumpExpression(*statement.if_.then_block, output, depth + 2);

      if (statement.if_.else_block != nullptr) {
        WriteIndent(output, depth + 1);
        output << "Else\n";
        DumpExpression(*statement.if_.else_block, output, depth + 2);
      }

      return;

    case StatementKind::kWhile:
      output << "WhileStatement\n";
      WriteIndent(output, depth + 1);
      output << "Condition\n";
      DumpExpression(*statement.while_.condition, output, depth + 2);
      WriteIndent(output, depth + 1);
      output << "Body\n";
      DumpExpression(*statement.while_.body, output, depth + 2);
      return;

    case StatementKind::kFor:
      output << "ForStatement\n";

      if (statement.for_.initializer != nullptr) {
        WriteIndent(output, depth + 1);
        output << "Initializer\n";
        DumpStatement(*statement.for_.initializer, output, depth + 2);
      }

      if (statement.for_.condition != nullptr) {
        WriteIndent(output, depth + 1);
        output << "Condition\n";
        DumpExpression(*statement.for_.condition, output, depth + 2);
      }

      if (statement.for_.increment != nullptr) {
        WriteIndent(output, depth + 1);
        output << "Increment\n";
        DumpExpression(*statement.for_.increment, output, depth + 2);
      }

      WriteIndent(output, depth + 1);
      output << "Body\n";
      DumpExpression(*statement.for_.body, output, depth + 2);
      return;

    case StatementKind::kBreak:
      output << "BreakStatement\n";
      return;

    case StatementKind::kContinue:
      output << "ContinueStatement\n";
      return;

    case StatementKind::kSwitch:
      output << "SwitchStatement\n";
      DumpExpression(*statement.switch_.data->value, output, depth + 1);

      for (const SwitchCase& case_ : statement.switch_.data->cases) {
        WriteIndent(output, depth + 1);
        output << "Case\n";
        DumpExpression(*case_.value, output, depth + 2);
        DumpExpression(*case_.body, output, depth + 2);
      }

      if (statement.switch_.data->default_body != nullptr) {
        WriteIndent(output, depth + 1);
        output << "Default\n";
        DumpExpression(*statement.switch_.data->default_body, output,
                       depth + 2);
      }

      return;
  }

  FELL_UNREACHABLE();
}

void DumpExpression(const Expression& expression, std::ostringstream& output,
                    usize depth) {
  WriteIndent(output, depth);

  switch (expression.kind) {
    case ExpressionKind::kInvalid:
      FELL_UNREACHABLE();

    case ExpressionKind::kBooleanLiteral:
      output << "BooleanLiteral "
             << (expression.boolean_literal.value ? "true" : "false") << '\n';
      return;

    case ExpressionKind::kIntegerLiteral:
      output << "IntegerLiteral " << expression.integer_literal.value;

      if (expression.integer_literal.explicit_type != Type::kInvalid) {
        output << " [" << ToString(expression.integer_literal.explicit_type)
               << "]";
      }

      output << '\n';
      return;

    case ExpressionKind::kFloatLiteral:
      output << "FloatLiteral " << expression.float_literal.value;

      if (expression.float_literal.explicit_type != Type::kInvalid) {
        output << " [" << ToString(expression.float_literal.explicit_type)
               << "]";
      }

      output << '\n';
      return;

    case ExpressionKind::kStringLiteral:
      output << "StringLiteral \"" << expression.string_literal.value << "\"\n";
      return;

    case ExpressionKind::kVariable:
      output << "VariableExpression " << expression.variable.name << '\n';
      return;

    case ExpressionKind::kAssignment:
      output << "AssignmentExpression " << expression.assignment.name << '\n';
      DumpExpression(*expression.assignment.value, output, depth + 1);
      return;

    case ExpressionKind::kCall:
      output << "CallExpression " << expression.call.data->callee << '\n';

      for (const Expression* argument : expression.call.data->arguments) {
        DumpExpression(*argument, output, depth + 1);
      }

      return;

    case ExpressionKind::kConditional:
      output << "ConditionalExpression\n";
      WriteIndent(output, depth + 1);
      output << "Condition\n";
      DumpExpression(*expression.conditional.condition, output, depth + 2);
      WriteIndent(output, depth + 1);
      output << "Then\n";
      DumpExpression(*expression.conditional.then_expression, output,
                     depth + 2);
      WriteIndent(output, depth + 1);
      output << "Else\n";
      DumpExpression(*expression.conditional.else_expression, output,
                     depth + 2);
      return;

    case ExpressionKind::kBlock:
      output << "BlockExpression\n";

      for (const Statement* statement : expression.block.body->statements) {
        DumpStatement(*statement, output, depth + 1);
      }

      if (expression.block.trailing_expression != nullptr) {
        WriteIndent(output, depth + 1);
        output << "TrailingExpression\n";
        DumpExpression(*expression.block.trailing_expression, output,
                       depth + 2);
      }

      return;

    case ExpressionKind::kUnary:
      output << "UnaryExpression (" << ToString(expression.unary.op) << ")\n";
      DumpExpression(*expression.unary.operand, output, depth + 1);
      return;

    case ExpressionKind::kBinary:
      output << "BinaryExpression (" << ToString(expression.binary.op) << ")\n";
      DumpExpression(*expression.binary.left, output, depth + 1);
      DumpExpression(*expression.binary.right, output, depth + 1);
      return;
  }

  FELL_UNREACHABLE();
}

}  // namespace

String DumpTokens(StringView source) {
  Lexer lexer{source};
  std::ostringstream output{};

  while (true) {
    const Token token{lexer.NextToken()};
    output << ToString(token.type) << " \"" << token.lexeme << "\"\n";

    if (token.type == TokenType::kEndOfFile) {
      break;
    }
  }

  return output.str();
}

String DumpAst(const CompilationUnit& unit) {
  std::ostringstream output{};
  output << "CompilationUnit\n";

  for (const Statement* statement : unit.statements) {
    DumpStatement(*statement, output, 1);
  }

  return output.str();
}

String DumpIr(const IrProgram& program) {
  std::ostringstream output{};

  auto dump_procedure = [&](StringView name, const IrProcedure& procedure) {
    output << name << ":\n";
    output << "  values:\n";

    for (usize index{0}; index < procedure.values.size(); ++index) {
      output << "    %" << index << ": "
             << ToString(procedure.values[index].type) << '\n';
    }

    output << "  blocks:\n";

    for (usize block_index{0}; block_index < procedure.blocks.size();
         ++block_index) {
      const IrBasicBlock& block{procedure.blocks[block_index]};
      output << "  B" << block_index;
      if (block_index == procedure.entry) {
        output << " [entry]";
      }
      output << ":\n";

      for (const IrInstruction& instruction : block.instructions) {
        output << "    ";

        switch (instruction.opcode) {
          case IrOpcode::kInvalid:
            FELL_UNREACHABLE();

          case IrOpcode::kConstant: {
            const Type type{
                GetIrValue(procedure, instruction.constant.destination).type,
            };
            output << "%" << instruction.constant.destination.value
                   << " = constant " << ToString(type) << " ";
            switch (type) {
              case Type::kBool:
                output << (instruction.constant.bool_value ? "true" : "false");
                break;
              case Type::kS8:
              case Type::kS16:
              case Type::kS32:
              case Type::kS64:
                output << instruction.constant.s64_value;
                break;
              case Type::kU8:
              case Type::kU16:
              case Type::kU32:
              case Type::kU64:
                output << instruction.constant.u64_value;
                break;
              case Type::kF32:
              case Type::kF64:
                output << instruction.constant.f64_value;
                break;
              case Type::kString:
                output
                    << '"'
                    << program
                           .string_constants[instruction.constant.string_value]
                    << '"';
                break;
              case Type::kUnit:
              case Type::kInvalid:
              case Type::kError:
                FELL_UNREACHABLE();
            }
            break;
          }

          case IrOpcode::kLoadGlobal:
            output << "%" << instruction.global.value.value
                   << " = load_global #" << instruction.global.global;
            break;
          case IrOpcode::kStoreGlobal:
            output << "store_global #" << instruction.global.global << ", %"
                   << instruction.global.value.value;
            break;
          case IrOpcode::kLoadLocal:
            output << "%" << instruction.local.value.value << " = load_local #"
                   << instruction.local.local;
            break;
          case IrOpcode::kStoreLocal:
            output << "store_local #" << instruction.local.local << ", %"
                   << instruction.local.value.value;
            break;
          case IrOpcode::kMove:
            output << "%" << instruction.move.destination.value << " = move %"
                   << instruction.move.source.value;
            break;
          case IrOpcode::kCall:
            if (instruction.call.has_destination) {
              output << "%" << instruction.call.destination.value << " = ";
            }
            output << "call fn#" << instruction.call.function << " (";
            for (u32 index{0}; index < instruction.call.argument_count;
                 ++index) {
              if (index != 0) {
                output << ", ";
              }
              output << "%"
                     << procedure
                            .call_arguments[instruction.call.argument_offset +
                                            index]
                            .value;
            }
            output << ")";
            break;
          case IrOpcode::kCallNative:
            output << "call_native #" << instruction.call_native.function
                   << " %" << instruction.call_native.argument.value;
            break;
          case IrOpcode::kConvert: {
            const Type source{
                GetIrValue(procedure, instruction.convert.source).type};
            const Type destination{
                GetIrValue(procedure, instruction.convert.destination).type,
            };
            output << "%" << instruction.convert.destination.value
                   << " = convert " << ToString(source) << " %"
                   << instruction.convert.source.value << " -> "
                   << ToString(destination);
            break;
          }
          case IrOpcode::kNegate:
          case IrOpcode::kLogicalNot: {
            const Type type{
                GetIrValue(procedure, instruction.unary.destination).type};
            output << "%" << instruction.unary.destination.value << " = "
                   << ToString(instruction.opcode) << ' ' << ToString(type)
                   << " %" << instruction.unary.operand.value;
            break;
          }
          case IrOpcode::kMultiply:
          case IrOpcode::kDivide:
          case IrOpcode::kAdd:
          case IrOpcode::kSubtract:
          case IrOpcode::kEqual:
          case IrOpcode::kNotEqual:
          case IrOpcode::kLess:
          case IrOpcode::kLessEqual:
          case IrOpcode::kGreater:
          case IrOpcode::kGreaterEqual: {
            const Type type{
                GetIrValue(procedure, instruction.binary.left).type};
            output << "%" << instruction.binary.destination.value << " = "
                   << ToString(instruction.opcode) << ' ' << ToString(type)
                   << " %" << instruction.binary.left.value << ", %"
                   << instruction.binary.right.value;
            break;
          }
        }

        output << '\n';
      }

      output << "    ";
      switch (block.terminator.kind) {
        case IrTerminatorKind::kInvalid:
          output << "<invalid terminator>";
          break;
        case IrTerminatorKind::kJump:
          output << "jump B" << block.terminator.jump.target;
          break;
        case IrTerminatorKind::kBranch:
          output << "branch %" << block.terminator.branch.condition.value
                 << ", B" << block.terminator.branch.true_target << ", B"
                 << block.terminator.branch.false_target;
          break;
        case IrTerminatorKind::kReturn:
          output << "return %" << block.terminator.return_.value.value;
          break;
        case IrTerminatorKind::kExit:
          output << "exit";
          break;
      }
      output << '\n';
    }
  };

  dump_procedure("main", program.main);

  for (const IrFunction& function : program.functions) {
    output << '\n';
    output << "function #" << function.id << " -> "
           << ToString(function.return_type) << '\n';
    dump_procedure("procedure", function.procedure);
  }

  return output.str();
}

}  // namespace fell

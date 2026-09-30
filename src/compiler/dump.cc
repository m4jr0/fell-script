#include "compiler/dump.h"

#include <sstream>

#include "compiler/lexer.h"
#include "compiler/token.h"
#include "compiler/type.h"
#include "core/assert.h"
#include "core/core.h"

namespace fell {
namespace {

constexpr usize kIndentSize{2};

StringView ToString(UnaryOperator op) {
  switch (op) {
    case UnaryOperator::kNegate:
      return "-";
    case UnaryOperator::kLogicalNot:
      return "!";
  }

  FELL_UNREACHABLE();
}

StringView ToString(BinaryOperator op) {
  switch (op) {
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
    case IrOpcode::kReturn:
      return "return";
  }

  FELL_UNREACHABLE();
}

void WriteIndent(std::ostringstream& output, usize depth) {
  output << String(depth * kIndentSize, ' ');
}

void DumpExpression(const Expression& expression, std::ostringstream& output,
                    usize depth);

void DumpStatement(const Statement& statement, std::ostringstream& output,
                   usize depth) {
  switch (statement.kind) {
    case StatementKind::kExpression:
      WriteIndent(output, depth);
      output << "ExpressionStatement\n";
      DumpExpression(*statement.expression.expression, output, depth + 1);
      return;

    case StatementKind::kVariableDeclaration: {
      const auto& declaration{statement.variable_declaration};

      WriteIndent(output, depth);
      output << "VariableDeclaration " << (declaration.is_mutable ? "mut " : "")
             << declaration.name;

      if (declaration.explicit_type != Type::kInvalid) {
        output << ": " << ToString(declaration.explicit_type);
      }

      output << '\n';
      DumpExpression(*declaration.initializer, output, depth + 1);
      return;
    }
  }

  FELL_UNREACHABLE();
}

void DumpExpression(const Expression& expression, std::ostringstream& output,
                    usize depth) {
  switch (expression.kind) {
    case ExpressionKind::kBooleanLiteral:
      WriteIndent(output, depth);
      output << "BooleanLiteral "
             << (expression.boolean_literal.value ? "true" : "false") << '\n';
      return;

    case ExpressionKind::kIntegerLiteral:
      WriteIndent(output, depth);
      output << "IntegerLiteral " << expression.integer_literal.value;
      if (expression.integer_literal.explicit_type != Type::kInvalid) {
        output << " [" << ToString(expression.integer_literal.explicit_type)
               << "]";
      }
      output << '\n';
      return;

    case ExpressionKind::kFloatLiteral:
      WriteIndent(output, depth);
      output << "FloatLiteral " << expression.float_literal.value;
      if (expression.float_literal.explicit_type != Type::kInvalid) {
        output << " [" << ToString(expression.float_literal.explicit_type)
               << "]";
      }
      output << '\n';
      return;

    case ExpressionKind::kStringLiteral:
      WriteIndent(output, depth);
      output << "StringLiteral \"" << expression.string_literal.value << "\"\n";
      return;

    case ExpressionKind::kVariable:
      WriteIndent(output, depth);
      output << "VariableExpression " << expression.variable.name << '\n';
      return;

    case ExpressionKind::kAssignment:
      WriteIndent(output, depth);
      output << "AssignmentExpression " << expression.assignment.name << '\n';
      DumpExpression(*expression.assignment.value, output, depth + 1);
      return;

    case ExpressionKind::kBlock:
      WriteIndent(output, depth);
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
      WriteIndent(output, depth);
      output << "UnaryExpression (" << ToString(expression.unary.op) << ")\n";
      DumpExpression(*expression.unary.operand, output, depth + 1);
      return;

    case ExpressionKind::kBinary:
      WriteIndent(output, depth);
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

  output << "values:\n";
  for (usize index{0}; index < program.values.size(); ++index) {
    output << "  %" << index << ": " << ToString(program.values[index].type)
           << '\n';
  }

  output << "\ninstructions:\n";

  for (const IrInstruction& instruction : program.instructions) {
    switch (instruction.opcode) {
      case IrOpcode::kConstant: {
        const Type type{
            GetIrValue(program, instruction.constant.destination).type,
        };

        output << "  %" << instruction.constant.destination.value
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
                << "\""
                << program.string_constants[instruction.constant.string_value]
                << "\"";
            break;

          case Type::kUnit:
          case Type::kInvalid:
          case Type::kError:
            FELL_UNREACHABLE();
        }

        output << '\n';
        break;
      }

      case IrOpcode::kLoadGlobal:
        output << "  %" << instruction.global.value.value << " = load_global #"
               << instruction.global.global << '\n';
        break;

      case IrOpcode::kStoreGlobal:
        output << "  store_global #" << instruction.global.global << ", %"
               << instruction.global.value.value << '\n';
        break;

      case IrOpcode::kLoadLocal:
        output << "  %" << instruction.local.value.value << " = load_local #"
               << instruction.local.local << '\n';
        break;

      case IrOpcode::kStoreLocal:
        output << "  store_local #" << instruction.local.local << ", %"
               << instruction.local.value.value << '\n';
        break;

      case IrOpcode::kConvert: {
        const Type source{
            GetIrValue(program, instruction.convert.source).type,
        };
        const Type destination{
            GetIrValue(program, instruction.convert.destination).type,
        };

        output << "  %" << instruction.convert.destination.value
               << " = convert " << ToString(source) << " %"
               << instruction.convert.source.value << " -> "
               << ToString(destination) << '\n';
        break;
      }

      case IrOpcode::kNegate:
      case IrOpcode::kLogicalNot: {
        const Type type{
            GetIrValue(program, instruction.unary.destination).type,
        };

        output << "  %" << instruction.unary.destination.value << " = "
               << ToString(instruction.opcode) << ' ' << ToString(type) << " %"
               << instruction.unary.operand.value << '\n';
        break;
      }

      case IrOpcode::kMultiply:
      case IrOpcode::kDivide:
      case IrOpcode::kAdd:
      case IrOpcode::kSubtract: {
        const Type type{
            GetIrValue(program, instruction.binary.destination).type,
        };

        output << "  %" << instruction.binary.destination.value << " = "
               << ToString(instruction.opcode) << ' ' << ToString(type) << " %"
               << instruction.binary.left.value << ", %"
               << instruction.binary.right.value << '\n';
        break;
      }

      case IrOpcode::kEqual:
      case IrOpcode::kNotEqual:
      case IrOpcode::kLess:
      case IrOpcode::kLessEqual:
      case IrOpcode::kGreater:
      case IrOpcode::kGreaterEqual: {
        const Type type{GetIrValue(program, instruction.binary.left).type};
        FELL_ASSERT(GetIrValue(program, instruction.binary.right).type == type);

        output << "  %" << instruction.binary.destination.value << " = "
               << ToString(instruction.opcode) << ' ' << ToString(type) << " %"
               << instruction.binary.left.value << ", %"
               << instruction.binary.right.value << '\n';
        break;
      }

      case IrOpcode::kReturn:
        output << "  return %" << instruction.return_.value.value << '\n';
        break;
    }
  }

  return output.str();
}

}  // namespace fell
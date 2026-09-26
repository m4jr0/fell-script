#include "compiler/dump.h"

#include <sstream>

#include "compiler/lexer.h"
#include "compiler/token.h"
#include "compiler/type.h"
#include "core/assert.h"
#include "core/core.h"

namespace fell {
namespace {

StringView ToString(BinaryOperator op) {
  switch (op) {
    case BinaryOperator::kAdd:
      return "+";
    case BinaryOperator::kSubtract:
      return "-";
  }

  FELL_UNREACHABLE();
}

void DumpExpression(const Expression& expression, std::ostringstream& output,
                    StringView indent) {
  switch (expression.kind) {
    case ExpressionKind::kIntegerLiteral:
      output << indent << "IntegerLiteral " << expression.integer_literal.value;
      if (expression.integer_literal.explicit_type != Type::kInvalid) {
        output << " [" << ToString(expression.integer_literal.explicit_type)
               << "]";
      }
      output << '\n';
      return;

    case ExpressionKind::kFloatLiteral:
      output << indent << "FloatLiteral " << expression.float_literal.value;
      if (expression.float_literal.explicit_type != Type::kInvalid) {
        output << " [" << ToString(expression.float_literal.explicit_type)
               << "]";
      }
      output << '\n';
      return;

    case ExpressionKind::kBinary:
      output << indent << "BinaryExpression (" << ToString(expression.binary.op)
             << ")\n";
      DumpExpression(*expression.binary.left, output, String{indent} + "  ");
      DumpExpression(*expression.binary.right, output, String{indent} + "  ");
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
    switch (statement->kind) {
      case StatementKind::kExpression:
        output << "  ExpressionStatement\n";
        DumpExpression(*statement->expression.expression, output, "    ");
        break;
    }
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

          case Type::kInvalid:
          case Type::kError:
            FELL_UNREACHABLE();
        }

        output << '\n';
        break;
      }

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

      case IrOpcode::kAdd:
      case IrOpcode::kSubtract: {
        const Type type{
            GetIrValue(program, instruction.binary.destination).type,
        };

        output << "  %" << instruction.binary.destination.value << " = "
               << (instruction.opcode == IrOpcode::kAdd ? "add " : "subtract ")
               << ToString(type) << " %" << instruction.binary.left.value
               << ", %" << instruction.binary.right.value << '\n';
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

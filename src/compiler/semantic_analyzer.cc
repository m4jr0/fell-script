#include "compiler/semantic_analyzer.h"

#include "core/assert.h"
#include "core/core.h"

namespace fell {

const ExpressionSemantics& SemanticModel::Get(
    const Expression& expression) const {
  const usize index{expression.id.value};

  FELL_ASSERT(index < expressions_.size());
  FELL_ASSERT(expressions_[index].type != Type::kInvalid);

  return expressions_[index];
}

void SemanticModel::Set(const Expression& expression,
                        ExpressionSemantics semantics) {
  const usize index{expression.id.value};

  if (expressions_.size() <= index) {
    expressions_.resize(index + 1);
  }

  expressions_[index] = semantics;
}

SemanticResult SemanticAnalyzer::Analyze(const CompilationUnit& unit) {
  SemanticResult result{};

  for (const Statement* statement : unit.statements) {
    AnalyzeStatement(*statement, result);
  }

  return result;
}

Type SemanticAnalyzer::GetIntegerLiteralType(
    const IntegerLiteralExpression& literal) {
  if (literal.explicit_type != Type::kInvalid) {
    if (!CanRepresentInteger(literal.explicit_type, literal.value)) {
      return Type::kInvalid;
    }

    return literal.explicit_type;
  }

  if (CanRepresentInteger(Type::kS32, literal.value)) {
    return Type::kS32;
  }

  if (CanRepresentInteger(Type::kS64, literal.value)) {
    return Type::kS64;
  }

  return Type::kInvalid;
}

Type SemanticAnalyzer::GetNegatedIntegerLiteralType(
    const IntegerLiteralExpression& literal) {
  if (literal.explicit_type != Type::kInvalid) {
    if (!IsSignedInteger(literal.explicit_type) ||
        !CanRepresentNegativeInteger(literal.explicit_type, literal.value)) {
      return Type::kInvalid;
    }

    return literal.explicit_type;
  }

  if (CanRepresentNegativeInteger(Type::kS32, literal.value)) {
    return Type::kS32;
  }

  if (CanRepresentNegativeInteger(Type::kS64, literal.value)) {
    return Type::kS64;
  }

  return Type::kInvalid;
}

Type SemanticAnalyzer::GetFloatLiteralType(
    const FloatLiteralExpression& literal) {
  if (literal.explicit_type != Type::kInvalid) {
    FELL_ASSERT(literal.explicit_type == Type::kF32 ||
                literal.explicit_type == Type::kF64);

    return literal.explicit_type;
  }

  return Type::kF64;
}

void SemanticAnalyzer::AnalyzeStatement(const Statement& statement,
                                        SemanticResult& result) {
  switch (statement.kind) {
    case StatementKind::kExpression:
      AnalyzeExpression(*statement.expression.expression, result);
      return;
  }

  FELL_UNREACHABLE();
}

void SemanticAnalyzer::AnalyzeExpression(const Expression& expression,
                                         SemanticResult& result) {
  switch (expression.kind) {
    case ExpressionKind::kIntegerLiteral: {
      const Type type{GetIntegerLiteralType(expression.integer_literal)};

      if (type == Type::kInvalid) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "integer literal is out of range",
        });
        return;
      }

      result.model.Set(expression, {
                                       .type = type,
                                   });
      return;
    }

    case ExpressionKind::kFloatLiteral: {
      const Type type{GetFloatLiteralType(expression.float_literal)};

      if (type == Type::kInvalid) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "float literal is out of range",
        });
        return;
      }

      result.model.Set(expression, {
                                       .type = type,
                                   });
      return;
    }

    case ExpressionKind::kUnary: {
      const auto& unary{expression.unary};

      if (unary.op == UnaryOperator::kNegate &&
          unary.operand->kind == ExpressionKind::kIntegerLiteral &&
          (unary.operand->integer_literal.explicit_type == Type::kInvalid ||
           IsSignedInteger(unary.operand->integer_literal.explicit_type))) {
        const Type type{
            GetNegatedIntegerLiteralType(unary.operand->integer_literal),
        };

        if (type == Type::kInvalid) {
          result.model.Set(*unary.operand, {
                                               .type = Type::kError,
                                           });
          result.model.Set(expression, {
                                           .type = Type::kError,
                                       });

          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message = "integer literal is out of range",
          });
          return;
        }

        // The literal's type is contextual here: its positive magnitude may
        // not be representable by the type, but the negated value is.
        result.model.Set(*unary.operand, {
                                             .type = type,
                                         });
        result.model.Set(expression, {
                                         .type = type,
                                     });
        return;
      }

      AnalyzeExpression(*unary.operand, result);
      const auto& operand{result.model.Get(*unary.operand)};

      if (operand.type == Type::kError) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });
        return;
      }

      switch (unary.op) {
        case UnaryOperator::kNegate:
          if (!IsSignedInteger(operand.type) &&
              !IsFloatingPoint(operand.type)) {
            result.model.Set(expression, {
                                             .type = Type::kError,
                                         });

            result.diagnostics.push_back({
                .severity = DiagnosticSeverity::kError,
                .message = "negation only applies to signed integers and "
                           "floating-point types",
            });
            return;
          }
          break;
      }

      result.model.Set(expression, {
                                       .type = operand.type,
                                   });
      return;
    }

    case ExpressionKind::kBinary: {
      const auto& binary{expression.binary};

      AnalyzeExpression(*binary.left, result);
      AnalyzeExpression(*binary.right, result);

      const auto& left{result.model.Get(*binary.left)};
      const auto& right{result.model.Get(*binary.right)};

      if (left.type == Type::kError || right.type == Type::kError) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });
        return;
      }

      const Type common_type{
          FindCommonNumericType(left.type, right.type),
      };

      if (common_type == Type::kInvalid) {
        result.model.Set(expression, {
                                         .type = Type::kError,
                                     });

        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "numeric operands have no lossless common type",
        });
        return;
      }

      result.model.Set(expression, {
                                       .type = common_type,
                                   });
      return;
    }
  }

  FELL_UNREACHABLE();
}

}  // namespace fell

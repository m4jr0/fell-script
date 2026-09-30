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

const StatementSemantics& SemanticModel::Get(const Statement& statement) const {
  const usize index{statement.id.value};
  FELL_ASSERT(index < statements_.size());
  FELL_ASSERT(statements_[index].type != Type::kInvalid);
  return statements_[index];
}

void SemanticModel::Set(const Statement& statement,
                        StatementSemantics semantics) {
  const usize index{statement.id.value};
  if (statements_.size() <= index) {
    statements_.resize(index + 1);
  }
  statements_[index] = semantics;
}

const SemanticAnalyzer::Symbol* SemanticAnalyzer::FindSymbol(
    StringView name) const {
  for (auto iterator{symbols_.rbegin()}; iterator != symbols_.rend();
       ++iterator) {
    if (iterator->name == name) {
      return &*iterator;
    }
  }
  return nullptr;
}

const SemanticAnalyzer::Symbol* SemanticAnalyzer::FindSymbolInCurrentScope(
    StringView name) const {
  for (auto iterator{symbols_.rbegin()}; iterator != symbols_.rend();
       ++iterator) {
    if (iterator->scope_depth < scope_depth_) {
      break;
    }
    if (iterator->name == name) {
      return &*iterator;
    }
  }
  return nullptr;
}

void SemanticAnalyzer::BeginScope() { ++scope_depth_; }

void SemanticAnalyzer::EndScope() {
  FELL_ASSERT(scope_depth_ > 0);
  while (!symbols_.empty() && symbols_.back().scope_depth == scope_depth_) {
    symbols_.pop_back();
  }
  --scope_depth_;
}

SemanticResult SemanticAnalyzer::Analyze(const CompilationUnit& unit) {
  symbols_.clear();
  scope_depth_ = 0;
  next_global_id_ = 0;
  next_local_id_ = 0;
  SemanticResult result{};
  for (const Statement* statement : unit.statements) {
    AnalyzeStatement(*statement, result);
  }
  result.model.global_count_ = next_global_id_;
  result.model.local_count_ = next_local_id_;
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

bool SemanticAnalyzer::TryApplyIntegerLiteralContext(
    const Expression& expression, Type expected_type, SemanticResult& result) {
  if (!IsSignedInteger(expected_type) && !IsUnsignedInteger(expected_type)) {
    return false;
  }

  if (expression.kind == ExpressionKind::kIntegerLiteral) {
    const auto& literal{expression.integer_literal};
    if (literal.explicit_type != Type::kInvalid ||
        !CanRepresentInteger(expected_type, literal.value)) {
      return false;
    }

    result.model.Set(expression, {.type = expected_type});
    return true;
  }

  if (expression.kind != ExpressionKind::kUnary ||
      expression.unary.op != UnaryOperator::kNegate ||
      expression.unary.operand->kind != ExpressionKind::kIntegerLiteral ||
      !IsSignedInteger(expected_type)) {
    return false;
  }

  const Expression& operand{*expression.unary.operand};
  const auto& literal{operand.integer_literal};
  if (literal.explicit_type != Type::kInvalid ||
      !CanRepresentNegativeInteger(expected_type, literal.value)) {
    return false;
  }

  // The positive magnitude may not fit in the signed type even though the
  // corresponding negative value does, such as -128s8.
  result.model.Set(operand, {.type = expected_type});
  result.model.Set(expression, {.type = expected_type});
  return true;
}

void SemanticAnalyzer::AnalyzeStatement(const Statement& statement,
                                        SemanticResult& result) {
  switch (statement.kind) {
    case StatementKind::kExpression:
      AnalyzeExpression(*statement.expression.expression, result);
      result.model.Set(
          statement,
          {.type = result.model.Get(*statement.expression.expression).type});
      return;

    case StatementKind::kVariableDeclaration: {
      const auto& declaration{statement.variable_declaration};
      if (FindSymbolInCurrentScope(declaration.name) != nullptr) {
        result.model.Set(statement, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "variable is already declared",
            .span = statement.span,
        });
        return;
      }

      if (declaration.explicit_type == Type::kInvalid ||
          !TryApplyIntegerLiteralContext(*declaration.initializer,
                                         declaration.explicit_type, result)) {
        AnalyzeExpression(*declaration.initializer, result);
      }
      const Type initializer_type{
          result.model.Get(*declaration.initializer).type};
      if (initializer_type == Type::kError) {
        result.model.Set(statement, {.type = Type::kError});
        return;
      }
      if (initializer_type == Type::kUnit) {
        result.model.Set(statement, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "variable initializer must produce a value",
            .span = statement.span,
        });
        return;
      }

      Type type{initializer_type};
      if (declaration.explicit_type != Type::kInvalid) {
        type = declaration.explicit_type;
        if (initializer_type != type &&
            (!IsNumericType(initializer_type) || !IsNumericType(type) ||
             !CanImplicitlyConvert(initializer_type, type))) {
          result.model.Set(statement, {.type = Type::kError});
          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message =
                  "initializer is not implicitly convertible to declared type",
              .span = statement.span,
          });
          return;
        }
      }

      VariableBinding binding{};
      if (scope_depth_ == 0) {
        FELL_ASSERT(next_global_id_ < kMaxValue<u32>);
        binding = {.storage = VariableStorage::kGlobal,
                   .slot = next_global_id_++};
      } else {
        FELL_ASSERT(next_local_id_ < kMaxValue<u32>);
        binding = {.storage = VariableStorage::kLocal,
                   .slot = next_local_id_++};
      }
      symbols_.push_back({.name = declaration.name,
                          .type = type,
                          .binding = binding,
                          .scope_depth = scope_depth_,
                          .is_mutable = declaration.is_mutable});
      result.model.Set(statement, {.type = type, .binding = binding});
      return;
    }
  }
  FELL_UNREACHABLE();
}

void SemanticAnalyzer::AnalyzeExpression(const Expression& expression,
                                         SemanticResult& result) {
  switch (expression.kind) {
    case ExpressionKind::kBooleanLiteral:
      result.model.Set(expression, {.type = Type::kBool});
      return;

    case ExpressionKind::kIntegerLiteral: {
      const Type type{GetIntegerLiteralType(expression.integer_literal)};
      if (type == Type::kInvalid) {
        result.model.Set(expression, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "integer literal is out of range",
            .span = expression.span,
        });
        return;
      }
      result.model.Set(expression, {.type = type});
      return;
    }

    case ExpressionKind::kFloatLiteral: {
      const Type type{GetFloatLiteralType(expression.float_literal)};
      if (type == Type::kInvalid) {
        result.model.Set(expression, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "float literal is out of range",
            .span = expression.span,
        });
        return;
      }
      result.model.Set(expression, {.type = type});
      return;
    }

    case ExpressionKind::kStringLiteral:
      result.model.Set(expression, {.type = Type::kString});
      return;

    case ExpressionKind::kVariable: {
      const Symbol* const symbol{FindSymbol(expression.variable.name)};
      if (symbol == nullptr) {
        result.model.Set(expression, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "undefined variable",
            .span = expression.span,
        });
        return;
      }
      result.model.Set(expression,
                       {.type = symbol->type, .binding = symbol->binding});
      return;
    }

    case ExpressionKind::kAssignment: {
      const Symbol* const symbol{FindSymbol(expression.assignment.name)};
      if (symbol == nullptr) {
        result.model.Set(expression, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "undefined variable",
            .span = expression.span,
        });
        return;
      }
      if (!symbol->is_mutable) {
        result.model.Set(expression, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "cannot assign to immutable variable",
            .span = expression.span,
        });
        return;
      }

      AnalyzeExpression(*expression.assignment.value, result);
      const Type value_type{
          result.model.Get(*expression.assignment.value).type};
      if (value_type == Type::kError) {
        result.model.Set(expression, {.type = Type::kError});
        return;
      }
      if (value_type != symbol->type &&
          (!IsNumericType(value_type) || !IsNumericType(symbol->type) ||
           !CanImplicitlyConvert(value_type, symbol->type))) {
        result.model.Set(expression, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message =
                "assigned value is not implicitly convertible to variable type",
            .span = expression.span,
        });
        return;
      }
      result.model.Set(expression, {.type = symbol->type,
                                    .operand_type = symbol->type,
                                    .binding = symbol->binding});
      return;
    }

    case ExpressionKind::kBlock: {
      BeginScope();
      bool has_error{false};
      for (const Statement* statement : expression.block.body->statements) {
        AnalyzeStatement(*statement, result);
        if (result.model.Get(*statement).type == Type::kError) {
          has_error = true;
        }
      }

      Type type{Type::kUnit};
      if (expression.block.trailing_expression != nullptr) {
        AnalyzeExpression(*expression.block.trailing_expression, result);
        type = result.model.Get(*expression.block.trailing_expression).type;
        has_error = has_error || type == Type::kError;
      }
      EndScope();
      result.model.Set(expression, {.type = has_error ? Type::kError : type});
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
          result.model.Set(*unary.operand, {.type = Type::kError});
          result.model.Set(expression, {.type = Type::kError});
          result.diagnostics.push_back({
              .severity = DiagnosticSeverity::kError,
              .message = "integer literal is out of range",
              .span = expression.span,
          });
          return;
        }

        // The literal's type is contextual here: its positive magnitude may
        // not be representable by the type, but the negated value is.
        result.model.Set(*unary.operand, {.type = type});
        result.model.Set(expression, {.type = type});
        return;
      }

      AnalyzeExpression(*unary.operand, result);
      const auto& operand{result.model.Get(*unary.operand)};
      if (operand.type == Type::kError) {
        result.model.Set(expression, {.type = Type::kError});
        return;
      }

      switch (unary.op) {
        case UnaryOperator::kNegate:
          if (!IsSignedInteger(operand.type) &&
              !IsFloatingPoint(operand.type)) {
            result.model.Set(expression, {.type = Type::kError});
            result.diagnostics.push_back({
                .severity = DiagnosticSeverity::kError,
                .message = "negation only applies to signed integers and "
                           "floating-point types",
                .span = expression.span,
            });
            return;
          }
          break;

        case UnaryOperator::kLogicalNot:
          if (operand.type != Type::kBool) {
            result.model.Set(expression, {.type = Type::kError});
            result.diagnostics.push_back({
                .severity = DiagnosticSeverity::kError,
                .message = "logical not only applies to bool",
                .span = expression.span,
            });
            return;
          }
          break;
      }

      result.model.Set(expression, {.type = operand.type});
      return;
    }

    case ExpressionKind::kBinary: {
      const auto& binary{expression.binary};
      AnalyzeExpression(*binary.left, result);
      AnalyzeExpression(*binary.right, result);

      const auto& left{result.model.Get(*binary.left)};
      const auto& right{result.model.Get(*binary.right)};
      if (left.type == Type::kError || right.type == Type::kError) {
        result.model.Set(expression, {.type = Type::kError});
        return;
      }

      const bool is_arithmetic{binary.op == BinaryOperator::kMultiply ||
                               binary.op == BinaryOperator::kDivide ||
                               binary.op == BinaryOperator::kAdd ||
                               binary.op == BinaryOperator::kSubtract};
      const bool is_equality{binary.op == BinaryOperator::kEqual ||
                             binary.op == BinaryOperator::kNotEqual};

      if (is_equality && left.type == Type::kBool &&
          right.type == Type::kBool) {
        result.model.Set(expression,
                         {.type = Type::kBool, .operand_type = Type::kBool});
        return;
      }

      if (binary.op == BinaryOperator::kAdd && left.type == Type::kString &&
          right.type == Type::kString) {
        result.model.Set(
            expression, {.type = Type::kString, .operand_type = Type::kString});
        return;
      }

      if (is_equality && left.type == Type::kString &&
          right.type == Type::kString) {
        result.model.Set(expression,
                         {.type = Type::kBool, .operand_type = Type::kString});
        return;
      }

      if (!IsNumericType(left.type) || !IsNumericType(right.type)) {
        result.model.Set(expression, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message =
                is_arithmetic
                    ? "arithmetic operators only apply to numeric types"
                : is_equality
                    ? "equality operands must both be bool, string, or numeric"
                    : "ordering operators only apply to numeric types",
            .span = expression.span,
        });
        return;
      }

      const Type common_type{FindCommonNumericType(left.type, right.type)};
      if (common_type == Type::kInvalid) {
        result.model.Set(expression, {.type = Type::kError});
        result.diagnostics.push_back({
            .severity = DiagnosticSeverity::kError,
            .message = "numeric operands have no lossless common type",
            .span = expression.span,
        });
        return;
      }

      result.model.Set(expression,
                       {.type = is_arithmetic ? common_type : Type::kBool,
                        .operand_type = common_type});
      return;
    }
  }

  FELL_UNREACHABLE();
}

}  // namespace fell

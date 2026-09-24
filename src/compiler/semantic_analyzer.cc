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

SemanticModel SemanticAnalyzer::Analyze(const CompilationUnit& unit) {
  SemanticModel model{};

  for (const Statement* statement : unit.statements) {
    AnalyzeStatement(*statement, model);
  }

  return model;
}

void SemanticAnalyzer::AnalyzeStatement(const Statement& statement,
                                        SemanticModel& model) {
  switch (statement.kind) {
    case StatementKind::kExpression:
      AnalyzeExpression(*statement.expression.expression, model);
      return;
  }

  FELL_UNREACHABLE();
}

void SemanticAnalyzer::AnalyzeExpression(const Expression& expression,
                                         SemanticModel& model) {
  switch (expression.kind) {
    case ExpressionKind::kIntegerLiteral:
      model.Set(expression, {
                                .type = Type::kS32,
                            });
      return;

    case ExpressionKind::kBinary: {
      const auto& binary{expression.binary};

      AnalyzeExpression(*binary.left, model);
      AnalyzeExpression(*binary.right, model);

      const auto& left{model.Get(*binary.left)};
      const auto& right{model.Get(*binary.right)};

      FELL_ASSERT(left.type == Type::kS32);
      FELL_ASSERT(right.type == Type::kS32);

      model.Set(expression, {
                                .type = Type::kS32,
                            });
      return;
    }
  }

  FELL_UNREACHABLE();
}

}  // namespace fell

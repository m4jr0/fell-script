#include "compiler/ast.h"

#include <utility>

#include "core/assert.h"
#include "core/core.h"
#include "core/memory.h"

namespace fell {

Expression* Ast::CreateIntegerLiteralExpression(u64 value, Type explicit_type) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kIntegerLiteral,
      .integer_literal =
          {
              .value = value,
              .explicit_type = explicit_type,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));

  return result;
}

Expression* Ast::CreateFloatLiteralExpression(f64 value, Type explicit_type) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kFloatLiteral,
      .float_literal =
          {
              .value = value,
              .explicit_type = explicit_type,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));

  return result;
}

Expression* Ast::CreateBinaryExpression(Expression* left, BinaryOperator op,
                                        Expression* right) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kBinary,
      .binary =
          {
              .left = left,
              .op = op,
              .right = right,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));

  return result;
}

Statement* Ast::CreateExpressionStatement(Expression* expression) {
  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);

  const StatementId id{
      .value = static_cast<u32>(statements_.size()),
  };

  auto statement{MakeUnique<Statement>(Statement{
      .id = id,
      .kind = StatementKind::kExpression,
      .expression =
          {
              .expression = expression,
          },
  })};

  auto* result{statement.get()};
  statements_.push_back(std::move(statement));

  return result;
}

}  // namespace fell

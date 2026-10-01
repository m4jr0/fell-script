#include "compiler/ast.h"

#include <utility>

#include "core/assert.h"
#include "core/memory.h"
#include "core/type.h"

namespace fell {

Expression* Ast::CreateBooleanLiteralExpression(bool value, SourceSpan span) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kBooleanLiteral,
      .span = span,
      .boolean_literal =
          {
              .value = value,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));
  return result;
}

Expression* Ast::CreateIntegerLiteralExpression(u64 value, Type explicit_type,
                                                SourceSpan span) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kIntegerLiteral,
      .span = span,
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

Expression* Ast::CreateFloatLiteralExpression(f64 value, Type explicit_type,
                                              SourceSpan span) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kFloatLiteral,
      .span = span,
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

Expression* Ast::CreateStringLiteralExpression(StringView value,
                                               SourceSpan span) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{.value = static_cast<u32>(expressions_.size())};

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kStringLiteral,
      .span = span,
      .string_literal =
          {
              .value = value,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));
  return result;
}

Expression* Ast::CreateVariableExpression(StringView name, SourceSpan span) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{.value = static_cast<u32>(expressions_.size())};

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kVariable,
      .span = span,
      .variable =
          {
              .name = name,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));
  return result;
}

Expression* Ast::CreateAssignmentExpression(StringView name, Expression* value,
                                            SourceSpan span) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{.value = static_cast<u32>(expressions_.size())};

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kAssignment,
      .span = span,
      .assignment =
          {
              .name = name,
              .value = value,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));
  return result;
}

Expression* Ast::CreateCallExpression(StringView callee,
                                      Vector<Expression*> arguments,
                                      SourceSpan span) {
  auto data{MakeUnique<CallData>(CallData{
      .callee = callee,
      .arguments = std::move(arguments),
  })};

  CallData* const data_ptr{data.get()};
  calls_.push_back(std::move(data));

  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{.value = static_cast<u32>(expressions_.size())};
  auto expression{
      MakeUnique<Expression>(Expression{.id = id,
                                        .kind = ExpressionKind::kCall,
                                        .span = span,
                                        .call = {
                                            .data = data_ptr,
                                        }})};

  Expression* const result{expression.get()};
  expressions_.push_back(std::move(expression));
  return result;
}

Expression* Ast::CreateConditionalExpression(Expression* condition,
                                             Expression* then_expression,
                                             Expression* else_expression,
                                             SourceSpan span) {
  FELL_ASSERT(condition != nullptr);
  FELL_ASSERT(then_expression != nullptr);
  FELL_ASSERT(else_expression != nullptr);
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kConditional,
      .span = span,
      .conditional =
          {
              .condition = condition,
              .then_expression = then_expression,
              .else_expression = else_expression,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));
  return result;
}

CompilationUnit* Ast::CreateCompilationUnit() {
  auto unit{MakeUnique<CompilationUnit>()};
  auto* result{unit.get()};
  compilation_units_.push_back(std::move(unit));
  return result;
}

Expression* Ast::CreateBlockExpression(CompilationUnit* body,
                                       Expression* trailing_expression,
                                       SourceSpan span) {
  FELL_ASSERT(body != nullptr);
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kBlock,
      .span = span,
      .block =
          {
              .body = body,
              .trailing_expression = trailing_expression,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));
  return result;
}

Expression* Ast::CreateUnaryExpression(UnaryOperator op, Expression* operand,
                                       SourceSpan span) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kUnary,
      .span = span,
      .unary =
          {
              .op = op,
              .operand = operand,
          },
  })};

  auto* result{expression.get()};
  expressions_.push_back(std::move(expression));
  return result;
}

Expression* Ast::CreateBinaryExpression(Expression* left, BinaryOperator op,
                                        Expression* right, SourceSpan span) {
  FELL_ASSERT(expressions_.size() <= kMaxValue<u32>);
  const ExpressionId id{
      .value = static_cast<u32>(expressions_.size()),
  };

  auto expression{MakeUnique<Expression>(Expression{
      .id = id,
      .kind = ExpressionKind::kBinary,
      .span = span,
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

Statement* Ast::CreateExpressionStatement(Expression* expression,
                                          SourceSpan span) {
  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);

  const StatementId id{
      .value = static_cast<u32>(statements_.size()),
  };

  auto statement{MakeUnique<Statement>(Statement{
      .id = id,
      .kind = StatementKind::kExpression,
      .span = span,
      .expression =
          {
              .expression = expression,
          },
  })};

  auto* result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

Statement* Ast::CreateVariableDeclarationStatement(StringView name,
                                                   Type explicit_type,
                                                   Expression* initializer,
                                                   bool is_mutable,
                                                   SourceSpan span) {
  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);

  const StatementId id{.value = static_cast<u32>(statements_.size())};
  auto statement{MakeUnique<Statement>(Statement{
      .id = id,
      .kind = StatementKind::kVariableDeclaration,
      .span = span,
      .variable_declaration =
          {
              .name = name,
              .explicit_type = explicit_type,
              .initializer = initializer,
              .is_mutable = is_mutable,
          },
  })};

  auto* result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

Statement* Ast::CreateFunctionDeclarationStatement(
    StringView name, Vector<FunctionParameter> parameters, Type return_type,
    Expression* body, SourceSpan span) {
  auto data{MakeUnique<FunctionData>(FunctionData{
      .name = name,
      .parameters = std::move(parameters),
      .return_type = return_type,
      .body = body,
  })};

  FunctionData* const data_ptr{data.get()};
  functions_.push_back(std::move(data));

  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);
  const StatementId id{.value = static_cast<u32>(statements_.size())};
  auto statement{
      MakeUnique<Statement>(Statement{
          .id = id,
          .kind = StatementKind::kFunctionDeclaration,
          .span = span,
          .function_declaration =
              {
                  .data = data_ptr,
              },
      }),
  };

  Statement* const result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

Statement* Ast::CreateReturnStatement(Expression* value, SourceSpan span) {
  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);
  const StatementId id{
      .value = static_cast<u32>(statements_.size()),
  };

  auto statement{MakeUnique<Statement>(Statement{
      .id = id,
      .kind = StatementKind::kReturn,
      .span = span,
      .return_ =
          {
              .value = value,
          },
  })};

  Statement* const result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

Statement* Ast::CreateIfStatement(Expression* condition, Expression* then_block,
                                  Expression* else_block, SourceSpan span) {
  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);
  const StatementId id{
      .value = static_cast<u32>(statements_.size()),
  };

  auto statement{
      MakeUnique<Statement>(Statement{
          .id = id,
          .kind = StatementKind::kIf,
          .span = span,
          .if_ =
              {
                  .condition = condition,
                  .then_block = then_block,
                  .else_block = else_block,
              },
      }),
  };

  auto* result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

Statement* Ast::CreateWhileStatement(Expression* condition, Expression* body,
                                     SourceSpan span) {
  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);
  const StatementId id{
      .value = static_cast<u32>(statements_.size()),
  };

  auto statement{MakeUnique<Statement>(Statement{
      .id = id,
      .kind = StatementKind::kWhile,
      .span = span,
      .while_ =
          {
              .condition = condition,
              .body = body,
          },
  })};

  auto* result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

Statement* Ast::CreateForStatement(Statement* initializer,
                                   Expression* condition, Expression* increment,
                                   Expression* body, SourceSpan span) {
  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);
  const StatementId id{
      .value = static_cast<u32>(statements_.size()),
  };

  auto statement{MakeUnique<Statement>(Statement{
      .id = id,
      .kind = StatementKind::kFor,
      .span = span,
      .for_ =
          {
              .initializer = initializer,
              .condition = condition,
              .increment = increment,
              .body = body,
          },
  })};

  auto* result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

Statement* Ast::CreateBreakStatement(SourceSpan span) {
  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);
  const StatementId id{
      .value = static_cast<u32>(statements_.size()),
  };

  auto statement{MakeUnique<Statement>(Statement{
      .id = id,
      .kind = StatementKind::kBreak,
      .span = span,
      .break_ = {},
  })};

  auto* result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

Statement* Ast::CreateContinueStatement(SourceSpan span) {
  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);
  const StatementId id{
      .value = static_cast<u32>(statements_.size()),
  };

  auto statement{MakeUnique<Statement>(Statement{
      .id = id,
      .kind = StatementKind::kContinue,
      .span = span,
      .continue_ = {},
  })};

  auto* result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

Statement* Ast::CreateSwitchStatement(Expression* value,
                                      Vector<SwitchCase> cases,
                                      Expression* default_body,
                                      SourceSpan span) {
  auto data{MakeUnique<SwitchData>(SwitchData{
      .value = value,
      .cases = std::move(cases),
      .default_body = default_body,
  })};

  SwitchData* data_ptr{data.get()};
  switches_.push_back(std::move(data));

  FELL_ASSERT(statements_.size() <= kMaxValue<u32>);
  const StatementId id{
      .value = static_cast<u32>(statements_.size()),
  };

  auto statement{MakeUnique<Statement>(Statement{
      .id = id,
      .kind = StatementKind::kSwitch,
      .span = span,
      .switch_ =
          {
              .data = data_ptr,
          },
  })};

  auto* result{statement.get()};
  statements_.push_back(std::move(statement));
  return result;
}

}  // namespace fell

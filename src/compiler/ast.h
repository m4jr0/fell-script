#pragma once

#include "core/memory.h"
#include "core/types.h"
#include "core/vector.h"

namespace fell {

enum class BinaryOperator {
  kAdd,
  kSubtract,
};

struct ExpressionId {
  u32 value;
};

enum class ExpressionKind {
  kIntegerLiteral,
  kBinary,
};

struct Expression;

struct BinaryExpression {
  Expression* left;
  BinaryOperator op;
  Expression* right;
};

struct IntegerLiteralExpression {
  s32 value;
};

struct Expression {
  ExpressionId id;
  ExpressionKind kind;

  union {
    IntegerLiteralExpression integer_literal;
    BinaryExpression binary;
  };
};

struct StatementId {
  u32 value;
};

enum class StatementKind {
  kExpression,
};

struct ExpressionStatement {
  Expression* expression;
};

struct Statement {
  StatementId id;
  StatementKind kind;

  union {
    ExpressionStatement expression;
  };
};

struct CompilationUnit {
  Vector<Statement*> statements;
};

class Ast {
 public:
  Expression* CreateIntegerLiteralExpression(s32 value);

  Expression* CreateBinaryExpression(Expression* left, BinaryOperator op,
                                     Expression* right);

  Statement* CreateExpressionStatement(Expression* expression);

 private:
  // TODO(m4jr0): Allocate AST nodes from an arena.
  Vector<UniquePtr<Expression>> expressions_;

  // TODO(m4jr0): Allocate AST nodes from an arena.
  Vector<UniquePtr<Statement>> statements_;
};

}  // namespace fell

#pragma once

#include "compiler/source_location.h"
#include "compiler/type.h"
#include "core/memory.h"
#include "core/types.h"
#include "core/vector.h"

namespace fell {

enum class UnaryOperator {
  kNegate,
  kLogicalNot,
};

enum class BinaryOperator {
  kMultiply,
  kDivide,
  kAdd,
  kSubtract,

  kEqual,
  kNotEqual,
  kLess,
  kLessEqual,
  kGreater,
  kGreaterEqual,
};

struct ExpressionId {
  u32 value;
};

enum class ExpressionKind {
  kBooleanLiteral,
  kIntegerLiteral,
  kFloatLiteral,
  kUnary,
  kBinary,
};

struct Expression;

struct BooleanLiteralExpression {
  bool value;
};

// No negatives.
// They are handled with the unary minus operator.
struct IntegerLiteralExpression {
  u64 value;
  Type explicit_type;
};

struct FloatLiteralExpression {
  f64 value;
  Type explicit_type;
};

struct UnaryExpression {
  UnaryOperator op;
  Expression* operand;
};

struct BinaryExpression {
  Expression* left;
  BinaryOperator op;
  Expression* right;
};

struct Expression {
  ExpressionId id;
  ExpressionKind kind;
  SourceSpan span;

  union {
    BooleanLiteralExpression boolean_literal;
    IntegerLiteralExpression integer_literal;
    FloatLiteralExpression float_literal;
    UnaryExpression unary;
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
  SourceSpan span;

  union {
    ExpressionStatement expression;
  };
};

struct CompilationUnit {
  Vector<Statement*> statements;
};

class Ast {
 public:
  Expression* CreateBooleanLiteralExpression(bool value, SourceSpan span);
  Expression* CreateIntegerLiteralExpression(u64 value, Type explicit_type,
                                             SourceSpan span);
  Expression* CreateFloatLiteralExpression(f64 value, Type explicit_type,
                                           SourceSpan span);

  Expression* CreateUnaryExpression(UnaryOperator op, Expression* operand,
                                    SourceSpan span);

  Expression* CreateBinaryExpression(Expression* left, BinaryOperator op,
                                     Expression* right, SourceSpan span);

  Statement* CreateExpressionStatement(Expression* expression, SourceSpan span);

 private:
  // TODO(m4jr0): Allocate AST nodes from an arena.
  Vector<UniquePtr<Expression>> expressions_;

  // TODO(m4jr0): Allocate AST nodes from an arena.
  Vector<UniquePtr<Statement>> statements_;
};

}  // namespace fell

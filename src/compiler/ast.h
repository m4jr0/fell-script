#pragma once

#include "compiler/source_location.h"
#include "compiler/type.h"
#include "core/memory.h"
#include "core/string.h"
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
  kStringLiteral,
  kVariable,
  kAssignment,
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

struct StringLiteralExpression {
  StringView value;
};

struct VariableExpression {
  StringView name;
};

struct AssignmentExpression {
  StringView name;
  Expression* value;
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
    StringLiteralExpression string_literal;
    VariableExpression variable;
    AssignmentExpression assignment;
    UnaryExpression unary;
    BinaryExpression binary;
  };
};

struct StatementId {
  u32 value;
};

enum class StatementKind {
  kExpression,
  kVariableDeclaration,
};

struct ExpressionStatement {
  Expression* expression;
};

struct VariableDeclarationStatement {
  StringView name;
  Type explicit_type;
  Expression* initializer;
  bool is_mutable;
};

struct Statement {
  StatementId id;
  StatementKind kind;
  SourceSpan span;

  union {
    ExpressionStatement expression;
    VariableDeclarationStatement variable_declaration;
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
  Expression* CreateStringLiteralExpression(StringView value, SourceSpan span);
  Expression* CreateVariableExpression(StringView name, SourceSpan span);
  Expression* CreateAssignmentExpression(StringView name, Expression* value,
                                         SourceSpan span);

  Expression* CreateUnaryExpression(UnaryOperator op, Expression* operand,
                                    SourceSpan span);

  Expression* CreateBinaryExpression(Expression* left, BinaryOperator op,
                                     Expression* right, SourceSpan span);

  Statement* CreateExpressionStatement(Expression* expression, SourceSpan span);
  Statement* CreateVariableDeclarationStatement(StringView name,
                                                Type explicit_type,
                                                Expression* initializer,
                                                bool is_mutable,
                                                SourceSpan span);

 private:
  // TODO(m4jr0): Allocate AST nodes from an arena.
  Vector<UniquePtr<Expression>> expressions_;

  // TODO(m4jr0): Allocate AST nodes from an arena.
  Vector<UniquePtr<Statement>> statements_;
};

}  // namespace fell

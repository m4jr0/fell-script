#pragma once

#include "compiler/source_location.h"
#include "compiler/type.h"
#include "core/memory.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vector.h"

namespace fell {

enum class UnaryOperator {
  kInvalid,

  kNegate,
  kLogicalNot,
};

enum class BinaryOperator {
  kInvalid,

  kLogicalAnd,
  kLogicalOr,
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
  kInvalid,

  kBooleanLiteral,
  kIntegerLiteral,
  kFloatLiteral,
  kStringLiteral,
  kVariable,
  kAssignment,
  kCall,
  kConditional,
  kBlock,
  kUnary,
  kBinary,
};

struct Expression;
struct Statement;
struct CompilationUnit;

struct BooleanLiteralExpression {
  bool value;
};

// No negatives.
// They are handled with the unary minus operator.
struct IntegerLiteralExpression {
  u64 value{0};
  Type explicit_type{Type::kInvalid};
};

struct FloatLiteralExpression {
  f64 value{0.0};
  Type explicit_type{Type::kInvalid};
};

struct StringLiteralExpression {
  StringView value;
};

struct VariableExpression {
  StringView name;
};

struct AssignmentExpression {
  StringView name;
  Expression* value{nullptr};
};

struct CallData {
  StringView callee;
  Vector<Expression*> arguments;
};

struct CallExpression {
  CallData* data;
};

struct ConditionalExpression {
  Expression* condition;
  Expression* then_expression;
  Expression* else_expression;
};

struct BlockExpression {
  CompilationUnit* body;
  Expression* trailing_expression;
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
    CallExpression call;
    ConditionalExpression conditional;
    BlockExpression block;
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
  kFunctionDeclaration,
  kReturn,
  kIf,
  kWhile,
  kFor,
  kBreak,
  kContinue,
  kSwitch,
};

struct EmptyStatement {};

struct ExpressionStatement {
  Expression* expression;
};

struct VariableDeclarationStatement {
  StringView name;
  Type explicit_type;
  Expression* initializer;
  bool is_mutable;
};

struct FunctionParameter {
  StringView name;
  Type type;
};

struct FunctionData {
  StringView name;
  Vector<FunctionParameter> parameters;
  Type return_type;
  Expression* body;
};

struct FunctionDeclarationStatement {
  FunctionData* data;
};
struct ReturnStatement {
  Expression* value;
};

struct IfStatement {
  Expression* condition;
  Expression* then_block;
  Expression* else_block;
};

struct WhileStatement {
  Expression* condition;
  Expression* body;
};

struct ForStatement {
  Statement* initializer;
  Expression* condition;
  Expression* increment;
  Expression* body;
};

struct SwitchCase {
  Expression* value;
  Expression* body;
};

struct SwitchData {
  Expression* value;
  Vector<SwitchCase> cases;
  Expression* default_body;
};

struct SwitchStatement {
  SwitchData* data;
};

struct Statement {
  StatementId id;
  StatementKind kind;
  SourceSpan span;

  union {
    ExpressionStatement expression;
    VariableDeclarationStatement variable_declaration;
    FunctionDeclarationStatement function_declaration;
    ReturnStatement return_;
    IfStatement if_;
    WhileStatement while_;
    ForStatement for_;
    EmptyStatement break_;
    EmptyStatement continue_;
    SwitchStatement switch_;
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
  Expression* CreateCallExpression(StringView callee,
                                   Vector<Expression*> arguments,
                                   SourceSpan span);
  Expression* CreateConditionalExpression(Expression* condition,
                                          Expression* then_expression,
                                          Expression* else_expression,
                                          SourceSpan span);
  Expression* CreateBlockExpression(CompilationUnit* body,
                                    Expression* trailing_expression,
                                    SourceSpan span);
  CompilationUnit* CreateCompilationUnit();

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
  Statement* CreateFunctionDeclarationStatement(
      StringView name, Vector<FunctionParameter> parameters, Type return_type,
      Expression* body, SourceSpan span);
  Statement* CreateReturnStatement(Expression* value, SourceSpan span);
  Statement* CreateIfStatement(Expression* condition, Expression* then_block,
                               Expression* else_block, SourceSpan span);
  Statement* CreateWhileStatement(Expression* condition, Expression* body,
                                  SourceSpan span);
  Statement* CreateForStatement(Statement* initializer, Expression* condition,
                                Expression* increment, Expression* body,
                                SourceSpan span);
  Statement* CreateBreakStatement(SourceSpan span);
  Statement* CreateContinueStatement(SourceSpan span);
  Statement* CreateSwitchStatement(Expression* value, Vector<SwitchCase> cases,
                                   Expression* default_body, SourceSpan span);

 private:
  // TODO(m4jr0): Allocate AST nodes from an arena.
  Vector<UniquePtr<Expression>> expressions_;

  // TODO(m4jr0): Allocate AST nodes from an arena.
  Vector<UniquePtr<Statement>> statements_;
  Vector<UniquePtr<CompilationUnit>> compilation_units_;
  Vector<UniquePtr<CallData>> calls_;
  Vector<UniquePtr<FunctionData>> functions_;
  Vector<UniquePtr<SwitchData>> switches_;
};

}  // namespace fell

#pragma once

#include "compiler/frontend/source_location.h"
#include "compiler/frontend/type.h"
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

inline constexpr u32 kInvalidExpressionId{static_cast<u32>(-1)};

struct ExpressionId {
  u32 value{kInvalidExpressionId};
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
  bool value{false};
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
  CallData* data{nullptr};
};

struct ConditionalExpression {
  Expression* condition{nullptr};
  Expression* then_expression{nullptr};
  Expression* else_expression{nullptr};
};

struct BlockExpression {
  CompilationUnit* body{nullptr};
  Expression* trailing_expression{nullptr};
};

struct UnaryExpression {
  UnaryOperator op{UnaryOperator::kInvalid};
  Expression* operand{nullptr};
};

struct BinaryExpression {
  Expression* left{nullptr};
  BinaryOperator op{BinaryOperator::kInvalid};
  Expression* right{nullptr};
};

struct Expression {
  ExpressionId id;
  ExpressionKind kind{ExpressionKind::kInvalid};
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

inline constexpr u32 kInvalidStatementId{static_cast<u32>(-1)};

struct StatementId {
  u32 value{kInvalidStatementId};
};

enum class StatementKind {
  kInvalid,

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
  Expression* expression{nullptr};
};

struct VariableDeclarationStatement {
  StringView name;
  Type explicit_type{Type::kInvalid};
  Expression* initializer{nullptr};
  bool is_mutable{false};
};

struct FunctionParameter {
  StringView name;
  Type type{Type::kInvalid};
};

struct FunctionData {
  StringView name;
  Vector<FunctionParameter> parameters;
  Type return_type{Type::kInvalid};
  Expression* body{nullptr};
};

struct FunctionDeclarationStatement {
  FunctionData* data{nullptr};
};

struct ReturnStatement {
  Expression* value{nullptr};
};

struct IfStatement {
  Expression* condition{nullptr};
  Expression* then_block{nullptr};
  Expression* else_block{nullptr};
};

struct WhileStatement {
  Expression* condition{nullptr};
  Expression* body{nullptr};
};

struct ForStatement {
  Statement* initializer{nullptr};
  Expression* condition{nullptr};
  Expression* increment{nullptr};
  Expression* body{nullptr};
};

struct SwitchCase {
  Expression* value{nullptr};
  Expression* body{nullptr};
};

struct SwitchData {
  Expression* value{nullptr};
  Vector<SwitchCase> cases;
  Expression* default_body{nullptr};
};

struct SwitchStatement {
  SwitchData* data{nullptr};
};

struct Statement {
  StatementId id;
  StatementKind kind{StatementKind::kInvalid};
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

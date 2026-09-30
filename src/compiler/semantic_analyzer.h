#pragma once

#include "compiler/ast.h"
#include "compiler/diagnostic.h"
#include "compiler/type.h"

namespace fell {

enum class VariableStorage : u8 {
  kGlobal,
  kLocal,
};

struct VariableBinding {
  VariableStorage storage{VariableStorage::kGlobal};
  u32 slot{0};
};

struct ExpressionSemantics {
  Type type{Type::kInvalid};
  Type operand_type{Type::kInvalid};
  VariableBinding binding{};
};

struct StatementSemantics {
  Type type{Type::kInvalid};
  VariableBinding binding{};
};

class SemanticModel {
 public:
  [[nodiscard]] const ExpressionSemantics& Get(
      const Expression& expression) const;
  [[nodiscard]] const StatementSemantics& Get(const Statement& statement) const;
  [[nodiscard]] u32 global_count() const { return global_count_; }
  [[nodiscard]] u32 local_count() const { return local_count_; }

 private:
  friend class SemanticAnalyzer;

  void Set(const Expression& expression, ExpressionSemantics semantics);
  void Set(const Statement& statement, StatementSemantics semantics);

  Vector<ExpressionSemantics> expressions_;
  Vector<StatementSemantics> statements_;
  u32 global_count_{0};
  u32 local_count_{0};
};

struct SemanticResult {
  SemanticModel model;
  Vector<Diagnostic> diagnostics;
};

class SemanticAnalyzer {
 public:
  SemanticResult Analyze(const CompilationUnit& unit);

 private:
  struct Symbol {
    StringView name;
    Type type;
    VariableBinding binding;
    u32 scope_depth;
    bool is_mutable;
  };

  static Type GetIntegerLiteralType(const IntegerLiteralExpression& literal);
  static Type GetNegatedIntegerLiteralType(
      const IntegerLiteralExpression& literal);
  static Type GetFloatLiteralType(const FloatLiteralExpression& literal);
  static bool TryApplyIntegerLiteralContext(const Expression& expression,
                                            Type expected_type,
                                            SemanticResult& result);

  const Symbol* FindSymbol(StringView name) const;
  const Symbol* FindSymbolInCurrentScope(StringView name) const;
  void BeginScope();
  void EndScope();
  void AnalyzeStatement(const Statement& statement, SemanticResult& result);
  void AnalyzeExpression(const Expression& expression, SemanticResult& result);

  Vector<Symbol> symbols_;
  u32 scope_depth_{0};
  u32 next_global_id_{0};
  u32 next_local_id_{0};
};

}  // namespace fell

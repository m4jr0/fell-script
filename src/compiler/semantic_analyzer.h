#pragma once

#include "compiler/ast.h"
#include "compiler/diagnostic.h"
#include "compiler/type.h"

namespace fell {

using GlobalId = u32;

struct ExpressionSemantics {
  Type type{Type::kInvalid};
  Type operand_type{Type::kInvalid};
  GlobalId global_id{0};
};

struct StatementSemantics {
  Type type{Type::kInvalid};
  GlobalId global_id{0};
};

class SemanticModel {
 public:
  [[nodiscard]] const ExpressionSemantics& Get(
      const Expression& expression) const;
  [[nodiscard]] const StatementSemantics& Get(const Statement& statement) const;
  [[nodiscard]] u32 global_count() const { return global_count_; }

 private:
  friend class SemanticAnalyzer;

  void Set(const Expression& expression, ExpressionSemantics semantics);
  void Set(const Statement& statement, StatementSemantics semantics);

  Vector<ExpressionSemantics> expressions_;
  Vector<StatementSemantics> statements_;
  u32 global_count_{0};
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
    GlobalId id;
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
  void AnalyzeStatement(const Statement& statement, SemanticResult& result);
  void AnalyzeExpression(const Expression& expression, SemanticResult& result);

  Vector<Symbol> symbols_;
};

}  // namespace fell

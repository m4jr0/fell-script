#pragma once

#include "compiler/ast.h"
#include "compiler/diagnostic.h"
#include "compiler/type.h"

namespace fell {

struct ExpressionSemantics {
  Type type{Type::kInvalid};
};

class SemanticModel {
 public:
  const ExpressionSemantics& Get(const Expression& expression) const;

 private:
  friend class SemanticAnalyzer;

  void Set(const Expression& expression, ExpressionSemantics semantics);

  Vector<ExpressionSemantics> expressions_;
};

struct SemanticResult {
  SemanticModel model;
  Vector<Diagnostic> diagnostics;
};

class SemanticAnalyzer {
 public:
  SemanticResult Analyze(const CompilationUnit& unit);

 private:
  static Type GetIntegerLiteralType(const IntegerLiteralExpression& literal);
  static Type GetFloatLiteralType(const FloatLiteralExpression& literal);

  void AnalyzeStatement(const Statement& statement, SemanticResult& result);
  void AnalyzeExpression(const Expression& expression, SemanticResult& result);
};

}  // namespace fell

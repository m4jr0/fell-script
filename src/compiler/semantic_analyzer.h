#pragma once

#include "compiler/ast.h"
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

class SemanticAnalyzer {
 public:
  SemanticModel Analyze(const CompilationUnit& unit);

 private:
  void AnalyzeStatement(const Statement& statement, SemanticModel& model);

  void AnalyzeExpression(const Expression& expression, SemanticModel& model);
};

}  // namespace fell

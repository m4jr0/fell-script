#include "compiler/semantic_analyzer.h"

namespace fell {

TypedIntegerExpression SemanticAnalyzer::Analyze(
    const IntegerExpression& expression) {
  return {
      .value = expression.value,
      .type = Type::kS32,
  };
}

}  // namespace fell
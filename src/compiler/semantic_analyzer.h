#pragma once

#include "compiler/ast.h"
#include "compiler/type.h"

namespace fell {

struct TypedIntegerExpression {
  s32 value;
  Type type;
};

class SemanticAnalyzer {
 public:
  TypedIntegerExpression Analyze(const IntegerExpression& expression);
};

}  // namespace fell
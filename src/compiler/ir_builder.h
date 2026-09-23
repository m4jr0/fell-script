#pragma once

#include "compiler/ir.h"
#include "compiler/semantic_analyzer.h"

namespace fell {

class IrBuilder {
 public:
  IrProgram Build(const TypedIntegerExpression& expression);
};

}  // namespace fell
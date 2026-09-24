#pragma once

#include "compiler/ir.h"
#include "compiler/semantic_analyzer.h"

namespace fell {

class IrBuilder {
 public:
  IrProgram Build(const CompilationUnit& unit, const SemanticModel& semantics);

 private:
  IrValueId BuildExpression(const Expression& expression,
                            const SemanticModel& semantics, IrProgram& program);

  IrValueId AllocateValue(IrProgram& program);
};

}  // namespace fell

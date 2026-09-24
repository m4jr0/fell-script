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

  IrValueId AllocateValue(IrProgram& program, Type type);

  IrValueId ConvertIfNeeded(IrValueId source, Type destination_type,
                            IrProgram& program);

  void EmitIntegerConstant(IrProgram& program, IrValueId destination, u64 value,
                           Type type);

  void EmitFloatConstant(IrProgram& program, IrValueId destination, f64 value,
                         Type type);
};

}  // namespace fell

#pragma once

#include "compiler/ir.h"
#include "compiler/semantic_analyzer.h"

namespace fell {

class IrBuilder {
 public:
  IrProgram Build(const CompilationUnit& unit, const SemanticModel& semantics,
                  bool return_last_expression = false);

 private:
  IrValueId BuildExpression(const Expression& expression,
                            const SemanticModel& semantics, IrProgram& program);

  IrValueId AllocateValue(IrProgram& program, Type type);

  IrValueId ConvertIfNeeded(IrValueId source, Type destination_type,
                            IrProgram& program);

  void EmitBooleanConstant(IrProgram& program, IrValueId destination,
                           bool value);

  void EmitIntegerConstant(IrProgram& program, IrValueId destination, u64 value,
                           Type type);

  void EmitNegatedIntegerConstant(IrProgram& program, IrValueId destination,
                                  u64 magnitude, Type type);

  void EmitFloatConstant(IrProgram& program, IrValueId destination, f64 value,
                         Type type);
};

}  // namespace fell

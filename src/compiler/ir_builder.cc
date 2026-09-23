#include "compiler/ir_builder.h"

namespace fell {

IrProgram IrBuilder::Build(const TypedIntegerExpression& expression) {
  IrProgram program;

  program.instructions.push_back({
      .opcode = IrOpcode::kConstantS32,
      .destination = 0,
      .immediate = expression.value,
  });

  program.instructions.push_back({
      .opcode = IrOpcode::kReturn,
      .destination = 0,
      .immediate = 0,
  });

  return program;
}

}  // namespace fell
#include "compiler/bytecode_compiler.h"

namespace fell {

Program BytecodeCompiler::Compile(const IrProgram& ir) {
  Program program;

  for (const IrInstruction& instruction : ir.instructions) {
    switch (instruction.opcode) {
      case IrOpcode::kConstantS32:
        program.instructions.push_back({
            .opcode = Opcode::kLoadImmediateS32,
            .destination = static_cast<u8>(instruction.destination),
            .immediate = instruction.immediate,
        });
        break;

      case IrOpcode::kReturn:
        program.instructions.push_back({
            .opcode = Opcode::kReturn,
            .destination = static_cast<u8>(instruction.destination),
            .immediate = 0,
        });
        break;
    }
  }

  return program;
}

}  // namespace fell
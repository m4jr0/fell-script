#include "runtime/vm.h"

namespace fell {

s32 Vm::Execute(const Program& program) {
  for (const Instruction& instruction : program.instructions) {
    switch (instruction.opcode) {
      case Opcode::kLoadImmediateS32:
        registers_[instruction.destination] = instruction.immediate;
        break;

      case Opcode::kReturn:
        return registers_[instruction.destination];
    }
  }

  return 0;
}

}  // namespace fell
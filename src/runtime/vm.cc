#include "runtime/vm.h"

#include "core/assert.h"

namespace fell {

s32 Vm::Execute(const BytecodeModule& module) {
  FELL_ASSERT(module.register_count <= kMaxRegisterCount);

  for (const Instruction& instruction : module.instructions) {
    switch (instruction.opcode) {
      case Opcode::kLoadImmediateS32:
        registers_[instruction.load_immediate_s32.destination] =
            instruction.load_immediate_s32.value;
        break;

      case Opcode::kAddS32:
        registers_[instruction.binary_s32.destination] =
            registers_[instruction.binary_s32.left] +
            registers_[instruction.binary_s32.right];
        break;

      case Opcode::kSubtractS32:
        registers_[instruction.binary_s32.destination] =
            registers_[instruction.binary_s32.left] -
            registers_[instruction.binary_s32.right];
        break;

      case Opcode::kReturn:
        return registers_[instruction.return_.source];
    }
  }

  FELL_UNREACHABLE();
}

}  // namespace fell

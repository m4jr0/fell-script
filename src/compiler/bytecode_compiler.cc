#include "compiler/bytecode_compiler.h"

#include "core/assert.h"
#include "core/core.h"

namespace fell {
namespace {
RegisterId ToRegister(IrValueId value) {
  FELL_ASSERT(value.value < kMaxRegisterCount);
  return static_cast<RegisterId>(value.value);
}
}  // namespace

BytecodeModule BytecodeCompiler::Compile(const IrProgram& ir) {
  FELL_ASSERT(ir.value_count <= kMaxRegisterCount);

  BytecodeModule module{
      .register_count = static_cast<u16>(ir.value_count),
  };

  for (const IrInstruction& instruction : ir.instructions) {
    switch (instruction.opcode) {
      case IrOpcode::kConstantS32:
        module.instructions.push_back({
            .opcode = Opcode::kLoadImmediateS32,
            .load_immediate_s32 =
                {
                    .destination =
                        ToRegister(instruction.constant_s32.destination),
                    .value = instruction.constant_s32.value,
                },
        });
        break;

      case IrOpcode::kAddS32:
        module.instructions.push_back({
            .opcode = Opcode::kAddS32,
            .binary_s32 =
                {
                    .destination =
                        ToRegister(instruction.binary_s32.destination),
                    .left = ToRegister(instruction.binary_s32.left),
                    .right = ToRegister(instruction.binary_s32.right),
                },
        });
        break;

      case IrOpcode::kSubtractS32:
        module.instructions.push_back({
            .opcode = Opcode::kSubtractS32,
            .binary_s32 =
                {
                    .destination =
                        ToRegister(instruction.binary_s32.destination),
                    .left = ToRegister(instruction.binary_s32.left),
                    .right = ToRegister(instruction.binary_s32.right),
                },
        });
        break;

      case IrOpcode::kReturn:
        module.instructions.push_back({
            .opcode = Opcode::kReturn,
            .return_ =
                {
                    .source = ToRegister(instruction.return_.value),
                },
        });
        break;
    }
  }

  return module;
}

}  // namespace fell

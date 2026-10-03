#pragma once

#include "bytecode/bytecode.h"
#include "compiler/ir.h"

namespace fell {

class BytecodeCompiler {
 public:
  [[nodiscard]] BytecodeModule Compile(const IrProgram& ir);

 private:
  struct ProcedureLayout {
    Vector<u32> block_targets;
    u32 end{0};
  };

  [[nodiscard]] ProcedureLayout ComputeProcedureLayout(
      const IrProcedure& procedure, u32 start) const;

  [[nodiscard]] ProcedureLayout CompileProcedure(const IrProcedure& procedure,
                                                 BytecodeModule& module);

  void CompileBlock(const IrProcedure& procedure, const IrBasicBlock& block,
                    const ProcedureLayout& layout, u32 argument_base,
                    BytecodeModule& module);

  void CompileInstruction(const IrProcedure& procedure,
                          const IrInstruction& instruction, u32 argument_base,
                          BytecodeModule& module);

  void CompileTerminator(const IrProcedure& procedure,
                         const IrTerminator& terminator,
                         const ProcedureLayout& layout, BytecodeModule& module);
};

}  // namespace fell

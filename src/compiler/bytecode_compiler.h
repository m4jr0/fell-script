#pragma once

#include "bytecode/bytecode.h"
#include "compiler/ir.h"

namespace fell {

class BytecodeCompiler {
 public:
  BytecodeModule Compile(const IrProgram& ir);
};

}  // namespace fell

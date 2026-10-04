#pragma once

#include "bytecode/bytecode.h"
#include "compiler/ir/ir.h"

namespace fell {

struct RegisterAllocation {
  Vector<RegisterId> registers;
  u16 register_count{0};
};

RegisterAllocation AllocateRegisters(const IrProcedure& procedure);

}  // namespace fell

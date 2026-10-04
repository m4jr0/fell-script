#pragma once

#include "compiler/ir/ir.h"

namespace fell {

// Verifies invariants that only hold after SSA construction has completed.
void VerifySsaIr(const IrProgram& program);

}  // namespace fell

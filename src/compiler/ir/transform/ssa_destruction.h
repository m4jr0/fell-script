#pragma once

#include "compiler/ir/ir.h"

namespace fell {

// Converts verified SSA IR back into non-SSA IR consumable by the current
// bytecode backend. Phi nodes become edge copies and explicit SSA parameters
// become entry-block local loads.
void DestroySsa(IrProcedure& procedure);

}  // namespace fell

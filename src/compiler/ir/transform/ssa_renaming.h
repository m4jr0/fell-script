#pragma once

#include "compiler/ir/ir.h"

namespace fell {

// Promotes mutable local slots that have StoreLocal definitions into SSA
// values. Phi nodes must already have been placed with PlacePhiNodes().
//
// Function parameters are deliberately left as local loads for now because the
// current IR has no explicit SSA parameter values yet.
void RenameLocalsToSsa(IrProcedure& procedure);

}  // namespace fell

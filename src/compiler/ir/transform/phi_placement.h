#pragma once

#include "compiler/ir/ir.h"
#include "core/vector.h"

namespace fell {

void PlacePhiNodes(IrProcedure& procedure,
                   const Vector<IrLocalId>& entry_defined_locals = {});

}  // namespace fell

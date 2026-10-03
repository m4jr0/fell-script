#pragma once

#include "compiler/ir.h"
#include "core/vector.h"

namespace fell {

struct ControlFlowGraph {
  Vector<Vector<IrBlockId>> predecessors;
  Vector<Vector<IrBlockId>> successors;
};

ControlFlowGraph BuildControlFlowGraph(const IrProcedure& procedure);

}  // namespace fell

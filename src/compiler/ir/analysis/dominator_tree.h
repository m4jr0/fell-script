#pragma once

#include "compiler/ir/analysis/control_flow_graph.h"
#include "compiler/ir/analysis/control_flow_traversal.h"
#include "compiler/ir/ir.h"
#include "core/vector.h"

namespace fell {

struct DominatorTree {
  Vector<IrBlockId> immediate_dominators;
  Vector<Vector<IrBlockId>> children;
};

DominatorTree ComputeDominatorTree(const IrProcedure& procedure,
                                   const ControlFlowGraph& graph,
                                   const ControlFlowTraversal& traversal);

}  // namespace fell

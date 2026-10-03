#pragma once

#include "compiler/ir/analysis/control_flow_graph.h"
#include "compiler/ir/analysis/dominator_tree.h"
#include "compiler/ir/ir.h"
#include "core/vector.h"

namespace fell {

struct DominanceFrontiers {
  Vector<Vector<IrBlockId>> blocks;
};

DominanceFrontiers ComputeDominanceFrontiers(
    const IrProcedure& procedure, const ControlFlowGraph& graph,
    const DominatorTree& dominator_tree);

}  // namespace fell

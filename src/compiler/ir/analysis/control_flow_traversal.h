#pragma once

#include "compiler/ir/analysis/control_flow_graph.h"
#include "compiler/ir/ir.h"
#include "core/vector.h"

namespace fell {

struct ControlFlowTraversal {
  Vector<bool> reachable;
  Vector<IrBlockId> reverse_postorder;
};

ControlFlowTraversal ComputeControlFlowTraversal(const IrProcedure& procedure,
                                                 const ControlFlowGraph& graph);

}  // namespace fell

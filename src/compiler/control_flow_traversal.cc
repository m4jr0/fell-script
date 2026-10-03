#include "compiler/control_flow_traversal.h"

#include <algorithm>

#include "core/assert.h"

namespace fell {
namespace {

void VisitBlock(IrBlockId block, const ControlFlowGraph& graph,
                Vector<bool>& visited, Vector<IrBlockId>& postorder) {
  FELL_ASSERT(block < graph.successors.size());

  if (visited[block]) {
    return;
  }

  visited[block] = true;

  for (IrBlockId successor : graph.successors[block]) {
    VisitBlock(successor, graph, visited, postorder);
  }

  postorder.push_back(block);
}

}  // namespace

ControlFlowTraversal ComputeControlFlowTraversal(
    const IrProcedure& procedure, const ControlFlowGraph& graph) {
  FELL_ASSERT(procedure.entry != kInvalidIrBlockId);
  FELL_ASSERT(procedure.entry < procedure.blocks.size());
  FELL_ASSERT(graph.predecessors.size() == procedure.blocks.size());
  FELL_ASSERT(graph.successors.size() == procedure.blocks.size());

  ControlFlowTraversal traversal{
      .reachable = Vector<bool>(procedure.blocks.size(), false),
      .reverse_postorder = {},
  };

  VisitBlock(procedure.entry, graph, traversal.reachable,
             traversal.reverse_postorder);

  std::reverse(traversal.reverse_postorder.begin(),
               traversal.reverse_postorder.end());

  return traversal;
}

}  // namespace fell

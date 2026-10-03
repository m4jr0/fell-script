#include "compiler/control_flow_graph.h"

#include "core/assert.h"

namespace fell {
namespace {

void AddEdge(ControlFlowGraph& graph, IrBlockId source, IrBlockId destination) {
  FELL_ASSERT(source < graph.successors.size());
  FELL_ASSERT(destination < graph.predecessors.size());

  graph.successors[source].push_back(destination);
  graph.predecessors[destination].push_back(source);
}

}  // namespace

ControlFlowGraph BuildControlFlowGraph(const IrProcedure& procedure) {
  ControlFlowGraph graph{
      .predecessors = Vector<Vector<IrBlockId>>(procedure.blocks.size()),
      .successors = Vector<Vector<IrBlockId>>(procedure.blocks.size()),
  };

  for (usize index{0}; index < procedure.blocks.size(); ++index) {
    const IrBlockId block_id{static_cast<IrBlockId>(index)};
    const IrTerminator& terminator{procedure.blocks[index].terminator};

    switch (terminator.kind) {
      case IrTerminatorKind::kInvalid:
        FELL_UNREACHABLE();

      case IrTerminatorKind::kJump:
        AddEdge(graph, block_id, terminator.jump.target);
        break;

      case IrTerminatorKind::kBranch:
        AddEdge(graph, block_id, terminator.branch.true_target);
        AddEdge(graph, block_id, terminator.branch.false_target);
        break;

      case IrTerminatorKind::kReturn:
      case IrTerminatorKind::kExit:
        break;
    }
  }

  return graph;
}

}  // namespace fell

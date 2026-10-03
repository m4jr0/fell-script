#include "compiler/ir/analysis/dominance_frontier.h"

#include <algorithm>

#include "core/assert.h"

namespace fell {
namespace {

void AddUnique(Vector<IrBlockId>& blocks, IrBlockId block) {
  if (std::find(blocks.begin(), blocks.end(), block) == blocks.end()) {
    blocks.push_back(block);
  }
}

}  // namespace

DominanceFrontiers ComputeDominanceFrontiers(
    const IrProcedure& procedure, const ControlFlowGraph& graph,
    const DominatorTree& dominator_tree) {
  FELL_ASSERT(graph.predecessors.size() == procedure.blocks.size());
  FELL_ASSERT(graph.successors.size() == procedure.blocks.size());
  FELL_ASSERT(dominator_tree.immediate_dominators.size() ==
              procedure.blocks.size());

  DominanceFrontiers frontiers{
      .blocks = Vector<Vector<IrBlockId>>(procedure.blocks.size()),
  };

  for (usize index{0}; index < procedure.blocks.size(); ++index) {
    const IrBlockId block{static_cast<IrBlockId>(index)};
    const IrBlockId immediate_dominator{
        dominator_tree.immediate_dominators[block],
    };

    if (immediate_dominator == kInvalidIrBlockId) {
      continue;
    }

    if (graph.predecessors[block].size() < 2) {
      continue;
    }

    for (IrBlockId predecessor : graph.predecessors[block]) {
      FELL_ASSERT(predecessor < procedure.blocks.size());

      if (dominator_tree.immediate_dominators[predecessor] ==
          kInvalidIrBlockId) {
        continue;
      }

      IrBlockId runner{predecessor};

      while (runner != immediate_dominator) {
        AddUnique(frontiers.blocks[runner], block);
        runner = dominator_tree.immediate_dominators[runner];
        FELL_ASSERT(runner != kInvalidIrBlockId);
      }
    }
  }

  return frontiers;
}

}  // namespace fell

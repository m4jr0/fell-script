#include "compiler/ir/analysis/dominator_tree.h"

#include "core/assert.h"

namespace fell {
namespace {

IrBlockId Intersect(IrBlockId left, IrBlockId right,
                    const Vector<IrBlockId>& immediate_dominators,
                    const Vector<u32>& reverse_postorder_positions) {
  FELL_ASSERT(left != kInvalidIrBlockId);
  FELL_ASSERT(right != kInvalidIrBlockId);

  while (left != right) {
    while (reverse_postorder_positions[left] >
           reverse_postorder_positions[right]) {
      left = immediate_dominators[left];
      FELL_ASSERT(left != kInvalidIrBlockId);
    }

    while (reverse_postorder_positions[right] >
           reverse_postorder_positions[left]) {
      right = immediate_dominators[right];
      FELL_ASSERT(right != kInvalidIrBlockId);
    }
  }

  return left;
}

}  // namespace

DominatorTree ComputeDominatorTree(const IrProcedure& procedure,
                                   const ControlFlowGraph& graph,
                                   const ControlFlowTraversal& traversal) {
  FELL_ASSERT(procedure.entry != kInvalidIrBlockId);
  FELL_ASSERT(procedure.entry < procedure.blocks.size());
  FELL_ASSERT(graph.predecessors.size() == procedure.blocks.size());
  FELL_ASSERT(graph.successors.size() == procedure.blocks.size());
  FELL_ASSERT(traversal.reachable.size() == procedure.blocks.size());
  FELL_ASSERT(!traversal.reverse_postorder.empty());
  FELL_ASSERT(traversal.reverse_postorder.front() == procedure.entry);

  DominatorTree tree{
      .immediate_dominators =
          Vector<IrBlockId>(procedure.blocks.size(), kInvalidIrBlockId),
      .children = Vector<Vector<IrBlockId>>(procedure.blocks.size()),
  };

  Vector<u32> reverse_postorder_positions(procedure.blocks.size(),
                                          kMaxValue<u32>);

  for (usize index{0}; index < traversal.reverse_postorder.size(); ++index) {
    const IrBlockId block{traversal.reverse_postorder[index]};
    FELL_ASSERT(block < procedure.blocks.size());
    FELL_ASSERT(traversal.reachable[block]);

    reverse_postorder_positions[block] = static_cast<u32>(index);
  }

  tree.immediate_dominators[procedure.entry] = procedure.entry;
  bool changed{true};

  while (changed) {
    changed = false;

    for (usize index{1}; index < traversal.reverse_postorder.size(); ++index) {
      const IrBlockId block{traversal.reverse_postorder[index]};
      IrBlockId new_immediate_dominator{kInvalidIrBlockId};

      for (IrBlockId predecessor : graph.predecessors[block]) {
        FELL_ASSERT(predecessor < procedure.blocks.size());

        if (tree.immediate_dominators[predecessor] == kInvalidIrBlockId) {
          continue;
        }

        if (new_immediate_dominator == kInvalidIrBlockId) {
          new_immediate_dominator = predecessor;
          continue;
        }

        new_immediate_dominator =
            Intersect(predecessor, new_immediate_dominator,
                      tree.immediate_dominators, reverse_postorder_positions);
      }

      FELL_ASSERT(new_immediate_dominator != kInvalidIrBlockId);

      if (tree.immediate_dominators[block] != new_immediate_dominator) {
        tree.immediate_dominators[block] = new_immediate_dominator;
        changed = true;
      }
    }
  }

  for (IrBlockId block : traversal.reverse_postorder) {
    if (block == procedure.entry) {
      continue;
    }

    const IrBlockId parent{tree.immediate_dominators[block]};
    FELL_ASSERT(parent != kInvalidIrBlockId);
    FELL_ASSERT(parent < tree.children.size());
    tree.children[parent].push_back(block);
  }

  return tree;
}

}  // namespace fell

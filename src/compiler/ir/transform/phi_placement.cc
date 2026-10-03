#include "compiler/ir/transform/phi_placement.h"

#include "compiler/ir/analysis/control_flow_graph.h"
#include "compiler/ir/analysis/control_flow_traversal.h"
#include "compiler/ir/analysis/dominance_frontier.h"
#include "compiler/ir/analysis/dominator_tree.h"
#include "core/assert.h"

namespace fell {
namespace {

struct LocalInformation {
  Vector<Vector<IrBlockId>> definition_blocks;
  Vector<Type> types;
};

IrValueId AllocateValue(IrProcedure& procedure, Type type) {
  FELL_ASSERT(type != Type::kInvalid);
  FELL_ASSERT(type != Type::kError);
  FELL_ASSERT(type != Type::kUnit);
  FELL_ASSERT(procedure.values.size() < kMaxValue<u32>);

  const IrValueId id{
      .value = static_cast<u32>(procedure.values.size()),
  };

  procedure.values.push_back({
      .type = type,
  });

  return id;
}

void AddUniqueBlock(Vector<IrBlockId>& blocks, IrBlockId block) {
  for (IrBlockId existing : blocks) {
    if (existing == block) {
      return;
    }
  }

  blocks.push_back(block);
}

void ObserveLocalType(LocalInformation& information, IrLocalId local,
                      Type type) {
  FELL_ASSERT(local < information.types.size());
  FELL_ASSERT(type != Type::kInvalid);
  FELL_ASSERT(type != Type::kError);
  FELL_ASSERT(type != Type::kUnit);

  if (information.types[local] == Type::kInvalid) {
    information.types[local] = type;
    return;
  }

  FELL_ASSERT(information.types[local] == type);
}

LocalInformation CollectLocalInformation(
    const IrProcedure& procedure,
    const Vector<IrLocalId>& entry_defined_locals) {
  LocalInformation information{
      .definition_blocks = Vector<Vector<IrBlockId>>(procedure.local_count),
      .types = Vector<Type>(procedure.local_count, Type::kInvalid),
  };

  Vector<bool> defined_at_entry(procedure.local_count, false);

  for (IrLocalId local : entry_defined_locals) {
    FELL_ASSERT(local < procedure.local_count);
    defined_at_entry[local] = true;
  }

  for (usize block_index{0}; block_index < procedure.blocks.size();
       ++block_index) {
    const IrBlockId block{static_cast<IrBlockId>(block_index)};

    for (const IrInstruction& instruction :
         procedure.blocks[block_index].instructions) {
      if (instruction.opcode != IrOpcode::kLoadLocal &&
          instruction.opcode != IrOpcode::kStoreLocal) {
        continue;
      }

      const IrLocalId local{instruction.local.local};
      FELL_ASSERT(local < procedure.local_count);

      ObserveLocalType(information, local,
                       GetIrValue(procedure, instruction.local.value).type);

      if (instruction.opcode == IrOpcode::kStoreLocal) {
        AddUniqueBlock(information.definition_blocks[local], block);
      }
    }
  }

  for (IrLocalId local{0}; local < procedure.local_count; ++local) {
    if (!defined_at_entry[local]) {
      continue;
    }

    // An entry-defined local only needs a type if it participates in the
    // procedure's local IR. Parameters that are never read or written can
    // remain absent from SSA promotion.
    if (information.types[local] != Type::kInvalid) {
      AddUniqueBlock(information.definition_blocks[local], procedure.entry);
    }
  }

  return information;
}

}  // namespace

void PlacePhiNodes(IrProcedure& procedure,
                   const Vector<IrLocalId>& entry_defined_locals) {
  const ControlFlowGraph graph{BuildControlFlowGraph(procedure)};
  const ControlFlowTraversal traversal{
      ComputeControlFlowTraversal(procedure, graph),
  };
  const DominatorTree dominator_tree{
      ComputeDominatorTree(procedure, graph, traversal),
  };
  const DominanceFrontiers dominance_frontiers{
      ComputeDominanceFrontiers(procedure, graph, dominator_tree),
  };

  LocalInformation information{
      CollectLocalInformation(procedure, entry_defined_locals),
  };

  for (IrLocalId local{0}; local < procedure.local_count; ++local) {
    if (information.definition_blocks[local].empty()) {
      continue;
    }

    FELL_ASSERT(information.types[local] != Type::kInvalid);

    Vector<bool> has_phi(procedure.blocks.size(), false);
    Vector<bool> in_worklist(procedure.blocks.size(), false);
    Vector<IrBlockId> worklist;

    for (IrBlockId block : information.definition_blocks[local]) {
      FELL_ASSERT(block < procedure.blocks.size());

      if (!traversal.reachable[block] || in_worklist[block]) {
        continue;
      }

      worklist.push_back(block);
      in_worklist[block] = true;
    }

    while (!worklist.empty()) {
      const IrBlockId definition_block{worklist.back()};
      worklist.pop_back();

      for (IrBlockId frontier : dominance_frontiers.blocks[definition_block]) {
        FELL_ASSERT(frontier < procedure.blocks.size());
        FELL_ASSERT(traversal.reachable[frontier]);

        if (has_phi[frontier]) {
          continue;
        }

        const IrValueId destination{
            AllocateValue(procedure, information.types[local]),
        };

        procedure.blocks[frontier].phis.push_back({
            .destination = destination,
            .local = local,
            .incoming_offset = 0,
            .incoming_count = 0,
        });

        has_phi[frontier] = true;

        // A phi is itself a definition. Its block may therefore require
        // another phi in its own dominance frontier.
        if (!in_worklist[frontier]) {
          worklist.push_back(frontier);
          in_worklist[frontier] = true;
        }
      }
    }
  }
}

}  // namespace fell

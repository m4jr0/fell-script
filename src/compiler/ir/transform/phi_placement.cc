#include "compiler/ir/transform/phi_placement.h"

#include "compiler/ir/analysis/control_flow_graph.h"
#include "compiler/ir/analysis/control_flow_traversal.h"
#include "compiler/ir/analysis/dominance_frontier.h"
#include "compiler/ir/analysis/dominator_tree.h"
#include "core/assert.h"

namespace fell {
namespace {

struct LocalDefinitions {
  Vector<Vector<IrBlockId>> blocks;
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

void ObserveLocalType(IrProcedure& procedure, IrLocalId local, Type type) {
  FELL_ASSERT(local < procedure.locals.size());
  FELL_ASSERT(type != Type::kInvalid);
  FELL_ASSERT(type != Type::kError);
  FELL_ASSERT(type != Type::kUnit);

  IrLocalMetadata& metadata{procedure.locals[local]};

  if (metadata.type == Type::kInvalid) {
    metadata.type = type;
    return;
  }

  FELL_ASSERT(metadata.type == type);
}

LocalDefinitions CollectLocalDefinitions(
    IrProcedure& procedure, const Vector<IrLocalId>& entry_defined_locals) {
  FELL_ASSERT(procedure.locals.size() == procedure.local_count);

  LocalDefinitions definitions{
      .blocks = Vector<Vector<IrBlockId>>(procedure.locals.size()),
  };

  Vector<bool> defined_at_entry(procedure.locals.size(), false);

  for (IrLocalId local : entry_defined_locals) {
    FELL_ASSERT(local < procedure.locals.size());
    defined_at_entry[local] = true;
  }

  for (usize block_index{0}; block_index < procedure.blocks.size();
       ++block_index) {
    const IrBlockId block{static_cast<IrBlockId>(block_index)};

    for (const IrInstruction& instruction :
         procedure.blocks[block_index].instructions) {
      if (!IsLocalAccess(instruction.opcode)) {
        continue;
      }

      const IrLocalId local{instruction.local.local};
      FELL_ASSERT(local < procedure.locals.size());

      ObserveLocalType(procedure, local,
                       GetIrValue(procedure, instruction.local.value).type);

      if (instruction.opcode == IrOpcode::kStoreLocal) {
        AddUniqueBlock(definitions.blocks[local], block);
      }
    }
  }

  for (IrLocalId local{0}; local < procedure.locals.size(); ++local) {
    if (!defined_at_entry[local]) {
      continue;
    }

    if (procedure.locals[local].type != Type::kInvalid) {
      AddUniqueBlock(definitions.blocks[local], procedure.entry);
    }
  }

  return definitions;
}

}  // namespace

void PlacePhiNodes(IrProcedure& procedure,
                   const Vector<IrLocalId>& entry_defined_locals) {
  if (procedure.locals.empty() && procedure.local_count != 0) {
    procedure.locals.resize(procedure.local_count);
  }

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

  const LocalDefinitions definitions{
      CollectLocalDefinitions(procedure, entry_defined_locals),
  };

  Vector<bool> has_phi(procedure.blocks.size());
  Vector<bool> in_worklist(procedure.blocks.size());
  Vector<IrBlockId> worklist;

  for (IrLocalId local{0}; local < procedure.locals.size(); ++local) {
    if (definitions.blocks[local].empty()) {
      continue;
    }

    std::fill(has_phi.begin(), has_phi.end(), false);
    std::fill(in_worklist.begin(), in_worklist.end(), false);
    worklist.clear();

    const Type type{procedure.locals[local].type};
    FELL_ASSERT(type != Type::kInvalid);

    for (IrBlockId block : definitions.blocks[local]) {
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
            AllocateValue(procedure, type),
        };

        procedure.blocks[frontier].phis.push_back({
            .destination = destination,
            .local = local,
            .incoming_offset = 0,
            .incoming_count = 0,
        });

        has_phi[frontier] = true;

        if (!in_worklist[frontier]) {
          worklist.push_back(frontier);
          in_worklist[frontier] = true;
        }
      }
    }
  }
}

}  // namespace fell

#include "compiler/ir/transform/ssa_renaming.h"

#include <utility>

#include "compiler/ir/analysis/control_flow_graph.h"
#include "compiler/ir/analysis/control_flow_traversal.h"
#include "compiler/ir/analysis/dominator_tree.h"
#include "core/assert.h"

namespace fell {
namespace {

IrValueId ResolveValue(const Vector<IrValueId>& aliases, IrValueId value) {
  FELL_ASSERT(value.value < aliases.size());

  while (aliases[value.value].value != value.value) {
    value = aliases[value.value];
    FELL_ASSERT(value.value < aliases.size());
  }

  return value;
}

void RewriteInstructionUses(IrProcedure& procedure, IrInstruction& instruction,
                            const Vector<IrValueId>& aliases) {
  switch (instruction.opcode) {
    case IrOpcode::kInvalid:
      FELL_UNREACHABLE();

    case IrOpcode::kConstant:
    case IrOpcode::kLoadGlobal:
    case IrOpcode::kLoadLocal:
      return;

    case IrOpcode::kConvert:
      instruction.convert.source =
          ResolveValue(aliases, instruction.convert.source);
      return;

    case IrOpcode::kStoreGlobal:
      instruction.global.value =
          ResolveValue(aliases, instruction.global.value);
      return;

    case IrOpcode::kStoreLocal:
      instruction.local.value = ResolveValue(aliases, instruction.local.value);
      return;

    case IrOpcode::kMove:
      instruction.move.source = ResolveValue(aliases, instruction.move.source);
      return;

    case IrOpcode::kCall:
      for (u32 index{0}; index < instruction.call.argument_count; ++index) {
        const u32 argument_index{instruction.call.argument_offset + index};
        FELL_ASSERT(argument_index < procedure.call_arguments.size());

        procedure.call_arguments[argument_index] =
            ResolveValue(aliases, procedure.call_arguments[argument_index]);
      }

      return;

    case IrOpcode::kCallNative:
      instruction.call_native.argument =
          ResolveValue(aliases, instruction.call_native.argument);

      return;

    case IrOpcode::kNegate:
    case IrOpcode::kLogicalNot:
      instruction.unary.operand =
          ResolveValue(aliases, instruction.unary.operand);

      return;

    case IrOpcode::kMultiply:
    case IrOpcode::kDivide:
    case IrOpcode::kAdd:
    case IrOpcode::kSubtract:
    case IrOpcode::kEqual:
    case IrOpcode::kNotEqual:
    case IrOpcode::kLess:
    case IrOpcode::kLessEqual:
    case IrOpcode::kGreater:
    case IrOpcode::kGreaterEqual:
      instruction.binary.left = ResolveValue(aliases, instruction.binary.left);
      instruction.binary.right =
          ResolveValue(aliases, instruction.binary.right);

      return;
  }

  FELL_UNREACHABLE();
}

void RewriteTerminatorUses(IrTerminator& terminator,
                           const Vector<IrValueId>& aliases) {
  switch (terminator.kind) {
    case IrTerminatorKind::kInvalid:
      FELL_UNREACHABLE();

    case IrTerminatorKind::kJump:
    case IrTerminatorKind::kExit:
      return;

    case IrTerminatorKind::kBranch:
      terminator.branch.condition =
          ResolveValue(aliases, terminator.branch.condition);
      return;

    case IrTerminatorKind::kReturn:
      terminator.return_.value =
          ResolveValue(aliases, terminator.return_.value);
      return;
  }

  FELL_UNREACHABLE();
}

bool HasPredecessor(const Vector<IrPhiIncoming>& incomings,
                    IrBlockId predecessor) {
  for (const IrPhiIncoming& incoming : incomings) {
    if (incoming.predecessor == predecessor) {
      return true;
    }
  }

  return false;
}

struct RenameState {
  IrProcedure& procedure;
  const ControlFlowGraph& graph;
  const DominatorTree& dominator_tree;
  const Vector<bool>& promotable_locals;

  Vector<Vector<IrValueId>> definitions;
  Vector<IrValueId> aliases;
  Vector<Vector<Vector<IrPhiIncoming>>> phi_incomings;
};

void RenameBlock(IrBlockId block, RenameState& state) {
  FELL_ASSERT(block < state.procedure.blocks.size());

  IrBasicBlock& basic_block{state.procedure.blocks[block]};
  Vector<IrLocalId> pushed_locals;

  for (usize phi_index{0}; phi_index < basic_block.phis.size(); ++phi_index) {
    const IrPhi& phi{basic_block.phis[phi_index]};

    if (phi.local == kInvalidIrLocalId) {
      continue;
    }

    FELL_ASSERT(phi.local < state.promotable_locals.size());

    if (!state.promotable_locals[phi.local]) {
      continue;
    }

    state.definitions[phi.local].push_back(phi.destination);
    pushed_locals.push_back(phi.local);
  }

  Vector<IrInstruction> instructions;
  instructions.reserve(basic_block.instructions.size());

  for (IrInstruction instruction : basic_block.instructions) {
    RewriteInstructionUses(state.procedure, instruction, state.aliases);

    if (instruction.opcode == IrOpcode::kLoadLocal) {
      const IrLocalId local{instruction.local.local};

      if (local < state.promotable_locals.size() &&
          state.promotable_locals[local]) {
        FELL_ASSERT(!state.definitions[local].empty());

        const IrValueId source{state.definitions[local].back()};
        FELL_ASSERT(instruction.local.value.value < state.aliases.size());
        state.aliases[instruction.local.value.value] =
            ResolveValue(state.aliases, source);
        continue;
      }
    }

    if (instruction.opcode == IrOpcode::kStoreLocal) {
      const IrLocalId local{instruction.local.local};

      if (local < state.promotable_locals.size() &&
          state.promotable_locals[local]) {
        const IrValueId value{
            ResolveValue(state.aliases, instruction.local.value),
        };

        state.definitions[local].push_back(value);
        pushed_locals.push_back(local);
        continue;
      }
    }

    instructions.push_back(instruction);
  }

  basic_block.instructions = std::move(instructions);
  RewriteTerminatorUses(basic_block.terminator, state.aliases);

  for (IrBlockId successor : state.graph.successors[block]) {
    FELL_ASSERT(successor < state.procedure.blocks.size());

    const IrBasicBlock& successor_block{state.procedure.blocks[successor]};

    for (usize phi_index{0}; phi_index < successor_block.phis.size();
         ++phi_index) {
      const IrPhi& phi{successor_block.phis[phi_index]};

      if (phi.local == kInvalidIrLocalId) {
        continue;
      }

      FELL_ASSERT(phi.local < state.promotable_locals.size());

      if (!state.promotable_locals[phi.local]) {
        continue;
      }

      FELL_ASSERT(!state.definitions[phi.local].empty());

      Vector<IrPhiIncoming>& incomings{
          state.phi_incomings[successor][phi_index],
      };

      if (HasPredecessor(incomings, block)) {
        continue;
      }

      incomings.push_back({
          .predecessor = block,
          .value =
              ResolveValue(state.aliases, state.definitions[phi.local].back()),
      });
    }
  }

  for (IrBlockId child : state.dominator_tree.children[block]) {
    RenameBlock(child, state);
  }

  for (auto iterator{pushed_locals.rbegin()}; iterator != pushed_locals.rend();
       ++iterator) {
    const IrLocalId local{*iterator};
    FELL_ASSERT(!state.definitions[local].empty());
    state.definitions[local].pop_back();
  }
}

}  // namespace

void RenameLocalsToSsa(IrProcedure& procedure) {
  const ControlFlowGraph graph{BuildControlFlowGraph(procedure)};
  const ControlFlowTraversal traversal{
      ComputeControlFlowTraversal(procedure, graph),
  };
  const DominatorTree dominator_tree{
      ComputeDominatorTree(procedure, graph, traversal),
  };

  Vector<bool> promotable_locals(procedure.locals.size(), false);

  for (const IrBasicBlock& block : procedure.blocks) {
    for (const IrInstruction& instruction : block.instructions) {
      if (instruction.opcode == IrOpcode::kStoreLocal) {
        FELL_ASSERT(instruction.local.local < promotable_locals.size());
        promotable_locals[instruction.local.local] = true;
      }
    }
  }

  Vector<IrValueId> aliases;
  aliases.reserve(procedure.values.size());

  for (usize index{0}; index < procedure.values.size(); ++index) {
    aliases.push_back({
        .value = static_cast<u32>(index),
    });
  }

  Vector<Vector<Vector<IrPhiIncoming>>> phi_incomings(procedure.blocks.size());

  for (usize block{0}; block < procedure.blocks.size(); ++block) {
    phi_incomings[block].resize(procedure.blocks[block].phis.size());
  }

  for (const IrParameter& parameter : procedure.parameters) {
    FELL_ASSERT(parameter.local < promotable_locals.size());
    promotable_locals[parameter.local] = true;
  }

  RenameState state{
      .procedure = procedure,
      .graph = graph,
      .dominator_tree = dominator_tree,
      .promotable_locals = promotable_locals,
      .definitions = Vector<Vector<IrValueId>>(procedure.locals.size()),
      .aliases = std::move(aliases),
      .phi_incomings = std::move(phi_incomings),
  };

  for (const IrParameter& parameter : procedure.parameters) {
    state.definitions[parameter.local].push_back(parameter.destination);
  }

  RenameBlock(procedure.entry, state);

  const Vector<IrPhiIncoming> original_phi_incomings{procedure.phi_incomings};
  procedure.phi_incomings.clear();

  for (usize block_index{0}; block_index < procedure.blocks.size();
       ++block_index) {
    IrBasicBlock& block{procedure.blocks[block_index]};

    for (usize phi_index{0}; phi_index < block.phis.size(); ++phi_index) {
      IrPhi& phi{block.phis[phi_index]};

      if (phi.local == kInvalidIrLocalId) {
        FELL_ASSERT(phi.incoming_offset <= original_phi_incomings.size());
        FELL_ASSERT(phi.incoming_count <=
                    original_phi_incomings.size() - phi.incoming_offset);

        const u32 old_offset{phi.incoming_offset};
        phi.incoming_offset = static_cast<u32>(procedure.phi_incomings.size());

        for (u32 index{0}; index < phi.incoming_count; ++index) {
          IrPhiIncoming incoming{original_phi_incomings[old_offset + index]};
          incoming.value = ResolveValue(state.aliases, incoming.value);
          procedure.phi_incomings.push_back(incoming);
        }

        continue;
      }

      if (phi.local >= promotable_locals.size() ||
          !promotable_locals[phi.local]) {
        continue;
      }

      Vector<IrPhiIncoming>& incomings{
          state.phi_incomings[block_index][phi_index],
      };

      FELL_ASSERT(procedure.phi_incomings.size() < kMaxValue<u32>);
      phi.incoming_offset = static_cast<u32>(procedure.phi_incomings.size());
      phi.incoming_count = static_cast<u32>(incomings.size());

      for (const IrPhiIncoming& incoming : incomings) {
        procedure.phi_incomings.push_back(incoming);
      }
    }
  }
}

}  // namespace fell

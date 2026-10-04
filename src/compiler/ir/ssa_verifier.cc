#include "compiler/ir/ssa_verifier.h"

#include "compiler/ir/analysis/control_flow_graph.h"
#include "compiler/ir/analysis/control_flow_traversal.h"
#include "compiler/ir/analysis/dominator_tree.h"
#include "core/assert.h"

namespace fell {
namespace {

enum class DefinitionKind : u8 {
  kInvalid,
  kParameter,
  kPhi,
  kInstruction,
};

struct Definition {
  DefinitionKind kind{DefinitionKind::kInvalid};
  IrBlockId block{kInvalidIrBlockId};
  u32 instruction_index{kMaxValue<u32>};
};

bool Dominates(IrBlockId dominator, IrBlockId block,
               const DominatorTree& tree) {
  FELL_ASSERT(dominator < tree.immediate_dominators.size());
  FELL_ASSERT(block < tree.immediate_dominators.size());

  while (block != dominator) {
    const IrBlockId parent{tree.immediate_dominators[block]};

    if (parent == kInvalidIrBlockId || parent == block) {
      return false;
    }

    block = parent;
  }

  return true;
}

void RecordDefinition(Vector<Definition>& definitions, IrValueId value,
                      Definition definition) {
  FELL_ASSERT(value.value < definitions.size());
  FELL_ASSERT(definitions[value.value].kind == DefinitionKind::kInvalid);
  definitions[value.value] = definition;
}

void VerifyUse(const IrProcedure& procedure,
               const Vector<Definition>& definitions,
               const DominatorTree& dominator_tree, IrValueId value,
               IrBlockId use_block, u32 instruction_index) {
  FELL_ASSERT(value.value < procedure.values.size());

  const Definition& definition{definitions[value.value]};
  FELL_ASSERT(definition.kind != DefinitionKind::kInvalid);
  FELL_ASSERT(Dominates(definition.block, use_block, dominator_tree));

  if (definition.block == use_block &&
      definition.kind == DefinitionKind::kInstruction) {
    FELL_ASSERT(definition.instruction_index < instruction_index);
  }
}

void VerifyInstructionUses(const IrProcedure& procedure,
                           const Vector<Definition>& definitions,
                           const DominatorTree& dominator_tree,
                           const IrInstruction& instruction, IrBlockId block,
                           u32 instruction_index) {
  auto verify = [&](IrValueId value) {
    VerifyUse(procedure, definitions, dominator_tree, value, block,
              instruction_index);
  };

  switch (instruction.opcode) {
    case IrOpcode::kInvalid:
      FELL_UNREACHABLE();

    case IrOpcode::kConstant:
    case IrOpcode::kLoadGlobal:
      return;

    case IrOpcode::kLoadLocal:
    case IrOpcode::kStoreLocal:
      // A completed SSA procedure must not contain local memory traffic.
      FELL_UNREACHABLE();

    case IrOpcode::kConvert:
      verify(instruction.convert.source);
      return;

    case IrOpcode::kStoreGlobal:
      verify(instruction.global.value);
      return;

    case IrOpcode::kMove:
      verify(instruction.move.source);
      return;

    case IrOpcode::kCall:
      for (u32 index{0}; index < instruction.call.argument_count; ++index) {
        const u32 argument_index{instruction.call.argument_offset + index};
        FELL_ASSERT(argument_index < procedure.call_arguments.size());
        verify(procedure.call_arguments[argument_index]);
      }
      return;

    case IrOpcode::kCallNative:
      verify(instruction.call_native.argument);
      return;

    case IrOpcode::kNegate:
    case IrOpcode::kLogicalNot:
      verify(instruction.unary.operand);
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
      verify(instruction.binary.left);
      verify(instruction.binary.right);
      return;
  }

  FELL_UNREACHABLE();
}

IrValueId GetInstructionDefinition(const IrInstruction& instruction) {
  switch (instruction.opcode) {
    case IrOpcode::kConstant:
      return instruction.constant.destination;
    case IrOpcode::kConvert:
      return instruction.convert.destination;
    case IrOpcode::kLoadGlobal:
      return instruction.global.value;
    case IrOpcode::kMove:
      return instruction.move.destination;
    case IrOpcode::kCall:
      return instruction.call.has_destination ? instruction.call.destination
                                              : IrValueId{};
    case IrOpcode::kNegate:
    case IrOpcode::kLogicalNot:
      return instruction.unary.destination;
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
      return instruction.binary.destination;

    case IrOpcode::kInvalid:
    case IrOpcode::kStoreGlobal:
    case IrOpcode::kLoadLocal:
    case IrOpcode::kStoreLocal:
    case IrOpcode::kCallNative:
      return {};
  }

  FELL_UNREACHABLE();
}

void VerifyProcedure(const IrProcedure& procedure) {
  const ControlFlowGraph graph{BuildControlFlowGraph(procedure)};
  const ControlFlowTraversal traversal{
      ComputeControlFlowTraversal(procedure, graph),
  };
  const DominatorTree dominator_tree{
      ComputeDominatorTree(procedure, graph, traversal),
  };

  Vector<Definition> definitions(procedure.values.size());

  for (const IrParameter& parameter : procedure.parameters) {
    FELL_ASSERT(parameter.destination.value < procedure.values.size());
    FELL_ASSERT(parameter.local < procedure.locals.size());

    RecordDefinition(definitions, parameter.destination,
                     {
                         .kind = DefinitionKind::kParameter,
                         .block = procedure.entry,
                         .instruction_index = 0,
                     });
  }

  for (usize block_index{0}; block_index < procedure.blocks.size();
       ++block_index) {
    if (!traversal.reachable[block_index]) {
      continue;
    }

    const IrBlockId block{static_cast<IrBlockId>(block_index)};
    const IrBasicBlock& basic_block{procedure.blocks[block_index]};

    for (const IrPhi& phi : basic_block.phis) {
      RecordDefinition(definitions, phi.destination,
                       {
                           .kind = DefinitionKind::kPhi,
                           .block = block,
                           .instruction_index = 0,
                       });
    }

    for (usize instruction_index{0};
         instruction_index < basic_block.instructions.size();
         ++instruction_index) {
      const IrValueId definition{
          GetInstructionDefinition(basic_block.instructions[instruction_index]),
      };

      if (definition.value == kInvalidIrValueId) {
        continue;
      }

      RecordDefinition(
          definitions, definition,
          {
              .kind = DefinitionKind::kInstruction,
              .block = block,
              .instruction_index = static_cast<u32>(instruction_index),
          });
    }
  }

  for (usize block_index{0}; block_index < procedure.blocks.size();
       ++block_index) {
    if (!traversal.reachable[block_index]) {
      continue;
    }

    const IrBlockId block{static_cast<IrBlockId>(block_index)};
    const IrBasicBlock& basic_block{procedure.blocks[block_index]};

    for (const IrPhi& phi : basic_block.phis) {
      FELL_ASSERT(phi.incoming_offset <= procedure.phi_incomings.size());
      FELL_ASSERT(phi.incoming_count <=
                  procedure.phi_incomings.size() - phi.incoming_offset);
      FELL_ASSERT(phi.incoming_count == graph.predecessors[block].size());

      Vector<bool> seen(graph.predecessors[block].size(), false);

      for (u32 index{0}; index < phi.incoming_count; ++index) {
        const IrPhiIncoming& incoming{
            procedure.phi_incomings[phi.incoming_offset + index],
        };

        usize predecessor_index{graph.predecessors[block].size()};

        for (usize candidate{0}; candidate < graph.predecessors[block].size();
             ++candidate) {
          if (graph.predecessors[block][candidate] == incoming.predecessor) {
            predecessor_index = candidate;
            break;
          }
        }

        FELL_ASSERT(predecessor_index < graph.predecessors[block].size());
        FELL_ASSERT(!seen[predecessor_index]);
        seen[predecessor_index] = true;

        VerifyUse(procedure, definitions, dominator_tree, incoming.value,
                  incoming.predecessor, kMaxValue<u32>);

        FELL_ASSERT(GetIrValue(procedure, incoming.value).type ==
                    GetIrValue(procedure, phi.destination).type);
      }
    }

    for (usize instruction_index{0};
         instruction_index < basic_block.instructions.size();
         ++instruction_index) {
      VerifyInstructionUses(procedure, definitions, dominator_tree,
                            basic_block.instructions[instruction_index], block,
                            static_cast<u32>(instruction_index));
    }

    switch (basic_block.terminator.kind) {
      case IrTerminatorKind::kInvalid:
        FELL_UNREACHABLE();

      case IrTerminatorKind::kJump:
      case IrTerminatorKind::kExit:
        break;

      case IrTerminatorKind::kBranch:
        VerifyUse(procedure, definitions, dominator_tree,
                  basic_block.terminator.branch.condition, block,
                  kMaxValue<u32>);
        break;

      case IrTerminatorKind::kReturn:
        VerifyUse(procedure, definitions, dominator_tree,
                  basic_block.terminator.return_.value, block, kMaxValue<u32>);
        break;
    }
  }
}

}  // namespace

void VerifySsaIr(const IrProgram& program) {
  VerifyProcedure(program.main);

  for (const IrFunction& function : program.functions) {
    VerifyProcedure(function.procedure);
  }
}

}  // namespace fell

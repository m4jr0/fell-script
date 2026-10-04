#include "compiler/ir/transform/ssa_destruction.h"

#include <utility>

#include "core/assert.h"

namespace fell {
namespace {

struct Copy {
  IrValueId destination;
  IrValueId source;
};

IrValueId AllocateValue(IrProcedure& procedure, Type type) {
  FELL_ASSERT(type != Type::kInvalid);
  FELL_ASSERT(type != Type::kError);
  FELL_ASSERT(type != Type::kUnit);
  FELL_ASSERT(procedure.values.size() < kMaxValue<u32>);

  const IrValueId value{static_cast<u32>(procedure.values.size())};
  procedure.values.push_back({.type = type});
  return value;
}

void EmitMove(Vector<IrInstruction>& instructions, IrValueId destination,
              IrValueId source) {
  if (destination.value == source.value) {
    return;
  }

  instructions.push_back({
      .opcode = IrOpcode::kMove,
      .move = {.destination = destination, .source = source},
  });
}

bool DestinationIsSource(const Vector<Copy>& copies, IrValueId destination) {
  for (const Copy& copy : copies) {
    if (copy.source.value == destination.value) {
      return true;
    }
  }
  return false;
}

void EmitParallelCopies(IrProcedure& procedure, Vector<IrInstruction>& output,
                        Vector<Copy> copies) {
  for (usize index{0}; index < copies.size();) {
    if (copies[index].destination.value == copies[index].source.value) {
      copies.erase(copies.begin() + index);
    } else {
      ++index;
    }
  }

  while (!copies.empty()) {
    bool emitted{false};

    for (usize index{0}; index < copies.size(); ++index) {
      if (DestinationIsSource(copies, copies[index].destination)) {
        continue;
      }

      EmitMove(output, copies[index].destination, copies[index].source);
      copies.erase(copies.begin() + index);
      emitted = true;
      break;
    }

    if (emitted) {
      continue;
    }

    const IrValueId cycle_value{copies[0].destination};
    const Type type{GetIrValue(procedure, cycle_value).type};
    const IrValueId temporary{AllocateValue(procedure, type)};
    EmitMove(output, temporary, cycle_value);

    for (Copy& copy : copies) {
      if (copy.source.value == cycle_value.value) {
        copy.source = temporary;
      }
    }
  }
}

void RetargetEdge(IrTerminator& terminator, IrBlockId old_target,
                  IrBlockId new_target) {
  switch (terminator.kind) {
    case IrTerminatorKind::kJump:
      FELL_ASSERT(terminator.jump.target == old_target);
      terminator.jump.target = new_target;
      return;

    case IrTerminatorKind::kBranch: {
      bool replaced{false};
      if (terminator.branch.true_target == old_target) {
        terminator.branch.true_target = new_target;
        replaced = true;
      }
      if (terminator.branch.false_target == old_target) {
        terminator.branch.false_target = new_target;
        replaced = true;
      }
      FELL_ASSERT(replaced);
      return;
    }

    case IrTerminatorKind::kInvalid:
    case IrTerminatorKind::kReturn:
    case IrTerminatorKind::kExit:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

void LowerPhis(IrProcedure& procedure) {
  const usize original_block_count{procedure.blocks.size()};

  for (usize block_index{0}; block_index < original_block_count;
       ++block_index) {
    if (procedure.blocks[block_index].phis.empty()) {
      continue;
    }

    const Vector<IrPhi> phis{procedure.blocks[block_index].phis};
    Vector<IrBlockId> predecessors;
    for (const IrPhi& phi : phis) {
      for (u32 index{0}; index < phi.incoming_count; ++index) {
        const IrPhiIncoming& incoming{
            procedure.phi_incomings[phi.incoming_offset + index]};
        bool found{false};
        for (IrBlockId predecessor : predecessors) {
          found = found || predecessor == incoming.predecessor;
        }
        if (!found) {
          predecessors.push_back(incoming.predecessor);
        }
      }
    }

    for (IrBlockId predecessor : predecessors) {
      Vector<Copy> copies;

      for (const IrPhi& phi : phis) {
        bool found{false};
        for (u32 index{0}; index < phi.incoming_count; ++index) {
          const IrPhiIncoming& incoming{
              procedure.phi_incomings[phi.incoming_offset + index]};
          if (incoming.predecessor != predecessor) {
            continue;
          }
          FELL_ASSERT(!found);
          copies.push_back(
              {.destination = phi.destination, .source = incoming.value});
          found = true;
        }
        FELL_ASSERT(found);
      }

      FELL_ASSERT(procedure.blocks.size() < kMaxValue<IrBlockId>);
      const IrBlockId edge_block{
          static_cast<IrBlockId>(procedure.blocks.size())};
      IrBasicBlock split{};
      EmitParallelCopies(procedure, split.instructions, std::move(copies));
      split.terminator = {
          .kind = IrTerminatorKind::kJump,
          .jump = {.target = static_cast<IrBlockId>(block_index)},
      };
      procedure.blocks.push_back(std::move(split));

      FELL_ASSERT(predecessor < procedure.blocks.size());
      RetargetEdge(procedure.blocks[predecessor].terminator,
                   static_cast<IrBlockId>(block_index), edge_block);
    }

    procedure.blocks[block_index].phis.clear();
  }

  procedure.phi_incomings.clear();
}

}  // namespace

void DestroySsa(IrProcedure& procedure) {
  if (!procedure.parameters.empty()) {
    IrBasicBlock& entry{procedure.blocks[procedure.entry]};
    Vector<IrInstruction> instructions;
    instructions.reserve(procedure.parameters.size() +
                         entry.instructions.size());

    for (const IrParameter& parameter : procedure.parameters) {
      instructions.push_back({
          .opcode = IrOpcode::kLoadLocal,
          .local = {.value = parameter.destination, .local = parameter.local},
      });
    }
    for (const IrInstruction& instruction : entry.instructions) {
      instructions.push_back(instruction);
    }
    entry.instructions = std::move(instructions);
    procedure.parameters.clear();
  }

  LowerPhis(procedure);
}

}  // namespace fell

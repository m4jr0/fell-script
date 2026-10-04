#include "compiler/backend/register_allocator.h"

#include <algorithm>

#include "compiler/ir/analysis/control_flow_graph.h"
#include "core/assert.h"

namespace fell {
namespace {

using ValueSet = Vector<bool>;

void AddUse(ValueSet& uses, const ValueSet& definitions, IrValueId value) {
  FELL_ASSERT(value.value < uses.size());
  if (!definitions[value.value]) {
    uses[value.value] = true;
  }
}

void AddDefinition(ValueSet& definitions, IrValueId value) {
  FELL_ASSERT(value.value < definitions.size());
  definitions[value.value] = true;
}

IrValueId GetDefinition(const IrInstruction& instruction) {
  switch (instruction.opcode) {
    case IrOpcode::kConstant:
      return instruction.constant.destination;
    case IrOpcode::kConvert:
      return instruction.convert.destination;
    case IrOpcode::kLoadGlobal:
      return instruction.global.value;
    case IrOpcode::kLoadLocal:
      return instruction.local.value;
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
    case IrOpcode::kStoreLocal:
    case IrOpcode::kCallNative:
      return {};
  }
  FELL_UNREACHABLE();
}

template <typename Function>
void ForEachUse(const IrProcedure& procedure, const IrInstruction& instruction,
                Function&& function) {
  switch (instruction.opcode) {
    case IrOpcode::kInvalid:
      FELL_UNREACHABLE();
    case IrOpcode::kConstant:
    case IrOpcode::kLoadGlobal:
    case IrOpcode::kLoadLocal:
      return;
    case IrOpcode::kConvert:
      function(instruction.convert.source);
      return;
    case IrOpcode::kStoreGlobal:
      function(instruction.global.value);
      return;
    case IrOpcode::kStoreLocal:
      function(instruction.local.value);
      return;
    case IrOpcode::kMove:
      function(instruction.move.source);
      return;
    case IrOpcode::kCall:
      for (u32 i{0}; i < instruction.call.argument_count; ++i) {
        function(
            procedure.call_arguments[instruction.call.argument_offset + i]);
      }
      return;
    case IrOpcode::kCallNative:
      function(instruction.call_native.argument);
      return;
    case IrOpcode::kNegate:
    case IrOpcode::kLogicalNot:
      function(instruction.unary.operand);
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
      function(instruction.binary.left);
      function(instruction.binary.right);
      return;
  }
  FELL_UNREACHABLE();
}

void AddInterference(Vector<ValueSet>& graph, IrValueId value,
                     const ValueSet& live) {
  if (value.value == kInvalidIrValueId) return;
  for (usize other{0}; other < live.size(); ++other) {
    if (!live[other] || other == value.value) continue;
    graph[value.value][other] = true;
    graph[other][value.value] = true;
  }
}

}  // namespace

RegisterAllocation AllocateRegisters(const IrProcedure& procedure) {
  const usize value_count{procedure.values.size()};
  const usize block_count{procedure.blocks.size()};
  const ControlFlowGraph cfg{BuildControlFlowGraph(procedure)};

  Vector<ValueSet> uses(block_count, ValueSet(value_count, false));
  Vector<ValueSet> definitions(block_count, ValueSet(value_count, false));

  for (usize b{0}; b < block_count; ++b) {
    for (const IrInstruction& instruction : procedure.blocks[b].instructions) {
      ForEachUse(procedure, instruction,
                 [&](IrValueId v) { AddUse(uses[b], definitions[b], v); });
      const IrValueId d{GetDefinition(instruction)};
      if (d.value != kInvalidIrValueId) AddDefinition(definitions[b], d);
    }
    const IrTerminator& t{procedure.blocks[b].terminator};
    if (t.kind == IrTerminatorKind::kBranch)
      AddUse(uses[b], definitions[b], t.branch.condition);
    if (t.kind == IrTerminatorKind::kReturn)
      AddUse(uses[b], definitions[b], t.return_.value);
  }

  Vector<ValueSet> live_in(block_count, ValueSet(value_count, false));
  Vector<ValueSet> live_out(block_count, ValueSet(value_count, false));
  bool changed{true};
  while (changed) {
    changed = false;
    for (usize reverse{block_count}; reverse > 0; --reverse) {
      const usize b{reverse - 1};
      ValueSet out(value_count, false);
      for (IrBlockId successor : cfg.successors[b]) {
        for (usize v{0}; v < value_count; ++v)
          out[v] = out[v] || live_in[successor][v];
      }
      ValueSet in{uses[b]};
      for (usize v{0}; v < value_count; ++v)
        in[v] = in[v] || (out[v] && !definitions[b][v]);
      if (out != live_out[b] || in != live_in[b]) {
        live_out[b] = std::move(out);
        live_in[b] = std::move(in);
        changed = true;
      }
    }
  }

  Vector<ValueSet> interference(value_count, ValueSet(value_count, false));
  for (usize b{0}; b < block_count; ++b) {
    ValueSet live{live_out[b]};
    const IrTerminator& t{procedure.blocks[b].terminator};
    if (t.kind == IrTerminatorKind::kBranch)
      live[t.branch.condition.value] = true;
    if (t.kind == IrTerminatorKind::kReturn) live[t.return_.value.value] = true;

    const auto& instructions{procedure.blocks[b].instructions};
    for (usize reverse{instructions.size()}; reverse > 0; --reverse) {
      const IrInstruction& instruction{instructions[reverse - 1]};
      const IrValueId d{GetDefinition(instruction)};
      if (d.value != kInvalidIrValueId) {
        AddInterference(interference, d, live);
        live[d.value] = false;
      }
      ForEachUse(procedure, instruction,
                 [&](IrValueId v) { live[v.value] = true; });
    }
  }

  Vector<u32> order(value_count);
  for (usize i{0}; i < value_count; ++i) order[i] = static_cast<u32>(i);
  std::sort(order.begin(), order.end(), [&](u32 a, u32 b) {
    return std::count(interference[a].begin(), interference[a].end(), true) >
           std::count(interference[b].begin(), interference[b].end(), true);
  });

  const RegisterId invalid{static_cast<RegisterId>(kMaxRegisterCount)};
  Vector<RegisterId> registers(value_count, invalid);
  u16 register_count{0};
  for (u32 value : order) {
    Vector<bool> unavailable(register_count, false);
    for (usize other{0}; other < value_count; ++other) {
      if (!interference[value][other] || registers[other] == invalid) continue;
      unavailable[registers[other]] = true;
    }
    RegisterId chosen{0};
    while (chosen < register_count && unavailable[chosen]) ++chosen;
    if (chosen == register_count) {
      FELL_ASSERT(register_count < kMaxRegisterCount);
      ++register_count;
    }
    registers[value] = chosen;
  }

  return {.registers = std::move(registers), .register_count = register_count};
}

}  // namespace fell

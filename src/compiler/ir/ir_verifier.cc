#include "compiler/ir/ir_verifier.h"

#include "compiler/frontend/type.h"
#include "core/assert.h"

namespace fell {
namespace {

void VerifyValue(const IrProcedure& procedure, IrValueId value) {
  FELL_ASSERT(value.value != kInvalidIrValueId);
  FELL_ASSERT(value.value < procedure.values.size());
  FELL_ASSERT(procedure.values[value.value].type != Type::kInvalid);
  FELL_ASSERT(procedure.values[value.value].type != Type::kError);
  FELL_ASSERT(procedure.values[value.value].type != Type::kUnit);
}

void VerifyBlock(const IrProcedure& procedure, IrBlockId block) {
  FELL_ASSERT(block != kInvalidIrBlockId);
  FELL_ASSERT(block < procedure.blocks.size());
}

void VerifySameType(const IrProcedure& procedure, IrValueId left,
                    IrValueId right) {
  VerifyValue(procedure, left);
  VerifyValue(procedure, right);
  FELL_ASSERT(GetIrValue(procedure, left).type ==
              GetIrValue(procedure, right).type);
}

void VerifyInstruction(const IrProgram& program, const IrProcedure& procedure,
                       const IrInstruction& instruction) {
  switch (instruction.opcode) {
    case IrOpcode::kInvalid:
      FELL_UNREACHABLE();

    case IrOpcode::kConstant:
      VerifyValue(procedure, instruction.constant.destination);
      if (GetIrValue(procedure, instruction.constant.destination).type ==
          Type::kString) {
        FELL_ASSERT(instruction.constant.string_value <
                    program.string_constants.size());
      }

      return;

    case IrOpcode::kConvert: {
      VerifyValue(procedure, instruction.convert.destination);
      VerifyValue(procedure, instruction.convert.source);

      const Type source{GetIrValue(procedure, instruction.convert.source).type};
      const Type destination{
          GetIrValue(procedure, instruction.convert.destination).type};

      FELL_ASSERT(source != destination);
      FELL_ASSERT(CanImplicitlyConvert(source, destination));
      return;
    }

    case IrOpcode::kLoadGlobal:
    case IrOpcode::kStoreGlobal:
      VerifyValue(procedure, instruction.global.value);
      FELL_ASSERT(instruction.global.global < program.global_count);
      return;

    case IrOpcode::kLoadLocal:
    case IrOpcode::kStoreLocal:
      VerifyValue(procedure, instruction.local.value);
      FELL_ASSERT(instruction.local.local < procedure.local_count);
      return;

    case IrOpcode::kMove:
      VerifySameType(procedure, instruction.move.destination,
                     instruction.move.source);
      return;

    case IrOpcode::kCall:
      FELL_ASSERT(instruction.call.function < program.functions.size());
      FELL_ASSERT(instruction.call.argument_offset <=
                  procedure.call_arguments.size());
      FELL_ASSERT(instruction.call.argument_count <=
                  procedure.call_arguments.size() -
                      instruction.call.argument_offset);

      for (u32 index{0}; index < instruction.call.argument_count; ++index) {
        VerifyValue(
            procedure,
            procedure.call_arguments[instruction.call.argument_offset + index]);
      }

      if (instruction.call.has_destination) {
        VerifyValue(procedure, instruction.call.destination);
      }

      return;

    case IrOpcode::kCallNative:
      VerifyValue(procedure, instruction.call_native.argument);

      FELL_ASSERT(instruction.call_native.argument_type ==
                  GetIrValue(procedure, instruction.call_native.argument).type);
      FELL_ASSERT(instruction.call_native.function != kInvalidNativeFunctionId);

      return;

    case IrOpcode::kNegate: {
      VerifySameType(procedure, instruction.unary.destination,
                     instruction.unary.operand);
      const Type type{GetIrValue(procedure, instruction.unary.operand).type};

      FELL_ASSERT(IsSignedInteger(type) || type == Type::kF32 ||
                  type == Type::kF64);

      return;
    }

    case IrOpcode::kLogicalNot:
      VerifySameType(procedure, instruction.unary.destination,
                     instruction.unary.operand);
      FELL_ASSERT(GetIrValue(procedure, instruction.unary.operand).type ==
                  Type::kBool);
      return;

    case IrOpcode::kMultiply:
    case IrOpcode::kDivide:
    case IrOpcode::kAdd:
    case IrOpcode::kSubtract:
      VerifySameType(procedure, instruction.binary.left,
                     instruction.binary.right);
      VerifySameType(procedure, instruction.binary.destination,
                     instruction.binary.left);
      return;

    case IrOpcode::kEqual:
    case IrOpcode::kNotEqual:
    case IrOpcode::kLess:
    case IrOpcode::kLessEqual:
    case IrOpcode::kGreater:
    case IrOpcode::kGreaterEqual:
      VerifySameType(procedure, instruction.binary.left,
                     instruction.binary.right);
      VerifyValue(procedure, instruction.binary.destination);
      FELL_ASSERT(GetIrValue(procedure, instruction.binary.destination).type ==
                  Type::kBool);
      return;
  }

  FELL_UNREACHABLE();
}

void VerifyTerminator(const IrProcedure& procedure,
                      const IrTerminator& terminator, bool allow_exit) {
  switch (terminator.kind) {
    case IrTerminatorKind::kInvalid:
      FELL_UNREACHABLE();

    case IrTerminatorKind::kJump:
      VerifyBlock(procedure, terminator.jump.target);
      return;

    case IrTerminatorKind::kBranch:
      VerifyValue(procedure, terminator.branch.condition);
      FELL_ASSERT(GetIrValue(procedure, terminator.branch.condition).type ==
                  Type::kBool);
      VerifyBlock(procedure, terminator.branch.true_target);
      VerifyBlock(procedure, terminator.branch.false_target);
      return;

    case IrTerminatorKind::kReturn:
      VerifyValue(procedure, terminator.return_.value);
      return;

    case IrTerminatorKind::kExit:
      FELL_ASSERT(allow_exit);
      return;
  }

  FELL_UNREACHABLE();
}

void VerifyProcedure(const IrProgram& program, const IrProcedure& procedure,
                     bool allow_exit) {
  FELL_ASSERT(!procedure.blocks.empty());
  VerifyBlock(procedure, procedure.entry);

  for (const IrValue& value : procedure.values) {
    FELL_ASSERT(value.type != Type::kInvalid);
    FELL_ASSERT(value.type != Type::kError);
    FELL_ASSERT(value.type != Type::kUnit);
  }

  for (const IrValueId argument : procedure.call_arguments) {
    VerifyValue(procedure, argument);
  }

  for (const IrBasicBlock& block : procedure.blocks) {
    for (const IrInstruction& instruction : block.instructions) {
      VerifyInstruction(program, procedure, instruction);
    }

    VerifyTerminator(procedure, block.terminator, allow_exit);
  }
}

}  // namespace

void VerifyIr(const IrProgram& program) {
  VerifyProcedure(program, program.main, true);

  for (usize index{0}; index < program.functions.size(); ++index) {
    const IrFunction& function{program.functions[index]};
    FELL_ASSERT(function.id == index);
    FELL_ASSERT(function.return_type != Type::kInvalid);
    FELL_ASSERT(function.return_type != Type::kError);

    for (IrLocalId parameter : function.parameter_local_slots) {
      FELL_ASSERT(parameter < function.procedure.local_count);
    }

    VerifyProcedure(program, function.procedure, false);
  }
}

}  // namespace fell

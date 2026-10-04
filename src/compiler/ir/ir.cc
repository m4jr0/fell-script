#include "compiler/ir/ir.h"

#include "core/assert.h"

namespace fell {

bool IsLocalAccess(IrOpcode opcode) {
  switch (opcode) {
    case IrOpcode::kLoadLocal:
    case IrOpcode::kStoreLocal:
      return true;

    case IrOpcode::kInvalid:
    case IrOpcode::kConstant:
    case IrOpcode::kConvert:
    case IrOpcode::kLoadGlobal:
    case IrOpcode::kStoreGlobal:
    case IrOpcode::kMove:
    case IrOpcode::kCall:
    case IrOpcode::kCallNative:
    case IrOpcode::kNegate:
    case IrOpcode::kLogicalNot:
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
      return false;
  }

  FELL_UNREACHABLE();
}

const IrValue& GetIrValue(const IrProcedure& procedure, IrValueId id) {
  FELL_ASSERT(id.value < procedure.values.size());
  return procedure.values[id.value];
}

}  // namespace fell

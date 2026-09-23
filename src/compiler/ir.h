#pragma once

#include "core/types.h"
#include "core/vector.h"

namespace fell {

enum class IrOpcode {
  kConstantS32,
  kReturn,
};

struct IrInstruction {
  IrOpcode opcode;
  u32 destination;
  s32 immediate;
};

struct IrProgram {
  Vector<IrInstruction> instructions;
};

}  // namespace fell
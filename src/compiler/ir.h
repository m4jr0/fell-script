#pragma once

#include "core/types.h"
#include "core/vector.h"

namespace fell {

struct IrValueId {
  u32 value;
};

enum class IrOpcode {
  kConstantS32,
  kAddS32,
  kSubtractS32,
  kReturn,
};

struct IrConstantS32 {
  IrValueId destination;
  s32 value;
};

struct IrBinaryS32 {
  IrValueId destination;
  IrValueId left;
  IrValueId right;
};

struct IrReturn {
  IrValueId value;
};

struct IrInstruction {
  IrOpcode opcode;

  union {
    IrConstantS32 constant_s32;
    IrBinaryS32 binary_s32;
    IrReturn return_;
  };
};

struct IrProgram {
  Vector<IrInstruction> instructions;
  u32 value_count{0};
};

}  // namespace fell

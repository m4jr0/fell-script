#pragma once

#include "core/types.h"
#include "core/vector.h"

namespace fell {

enum class Opcode : u8 {
  kLoadImmediateS32,
  kAddS32,
  kSubtractS32,
  kReturn,
};

using RegisterId = u16;

inline constexpr u32 kMaxRegisterCount{256};

struct LoadImmediateS32Instruction {
  RegisterId destination;
  s32 value;
};

struct BinaryS32Instruction {
  RegisterId destination;
  RegisterId left;
  RegisterId right;
};

struct ReturnInstruction {
  RegisterId source;
};

struct Instruction {
  Opcode opcode;

  union {
    LoadImmediateS32Instruction load_immediate_s32;
    BinaryS32Instruction binary_s32;
    ReturnInstruction return_;
  };
};

struct BytecodeModule {
  Vector<Instruction> instructions;
  u16 register_count{0};
};

}  // namespace fell

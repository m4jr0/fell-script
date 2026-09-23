#pragma once

#include "core/types.h"
#include "core/vector.h"

namespace fell {

enum class Opcode : u8 {
  kLoadImmediateS32,
  kReturn,
};

struct Instruction {
  Opcode opcode;
  u8 destination;
  s32 immediate;
};

struct Program {
  Vector<Instruction> instructions;
};

}  // namespace fell
#pragma once

#include "core/types.h"
#include "core/vector.h"
#include "runtime/value.h"

namespace fell {

enum class Opcode : u8 {
  kLoadImmediate,

  kConvertS8ToS16,
  kConvertS8ToS32,
  kConvertS8ToS64,
  kConvertS8ToF32,
  kConvertS8ToF64,

  kConvertS16ToS32,
  kConvertS16ToS64,
  kConvertS16ToF32,
  kConvertS16ToF64,

  kConvertS32ToS64,
  kConvertS32ToF64,

  kConvertU8ToS16,
  kConvertU8ToS32,
  kConvertU8ToS64,
  kConvertU8ToU16,
  kConvertU8ToU32,
  kConvertU8ToU64,
  kConvertU8ToF32,
  kConvertU8ToF64,

  kConvertU16ToS32,
  kConvertU16ToS64,
  kConvertU16ToU32,
  kConvertU16ToU64,
  kConvertU16ToF32,
  kConvertU16ToF64,

  kConvertU32ToS64,
  kConvertU32ToU64,
  kConvertU32ToF64,

  kConvertF32ToF64,

  kAddS8,
  kAddS16,
  kAddS32,
  kAddS64,
  kAddU8,
  kAddU16,
  kAddU32,
  kAddU64,
  kAddF32,
  kAddF64,

  kSubtractS8,
  kSubtractS16,
  kSubtractS32,
  kSubtractS64,
  kSubtractU8,
  kSubtractU16,
  kSubtractU32,
  kSubtractU64,
  kSubtractF32,
  kSubtractF64,

  kReturn,
};

using RegisterId = u16;

inline constexpr u32 kMaxRegisterCount{256};

struct LoadImmediateInstruction {
  RegisterId destination;
  Value value;
};

struct ConvertInstruction {
  RegisterId destination;
  RegisterId source;
};

struct BinaryInstruction {
  RegisterId destination;
  RegisterId left;
  RegisterId right;
};

struct ReturnInstruction {
  RegisterId source;
  ValueType type;
};

struct Instruction {
  Opcode opcode;

  union {
    LoadImmediateInstruction load_immediate;
    ConvertInstruction convert;
    BinaryInstruction binary;
    ReturnInstruction return_;
  };
};

struct BytecodeModule {
  Vector<Instruction> instructions;
  u16 register_count{0};
};

}  // namespace fell
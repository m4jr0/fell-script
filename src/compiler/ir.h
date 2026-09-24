#pragma once

#include "compiler/type.h"
#include "core/types.h"
#include "core/vector.h"

namespace fell {

struct IrValueId {
  u32 value;
};

enum class IrOpcode {
  kConstant,
  kConvert,
  kAdd,
  kSubtract,
  kReturn,
};

struct IrConstant {
  IrValueId destination;

  union {
    s64 s64_value;
    u64 u64_value;
    f64 f64_value;
  };
};

struct IrConvert {
  IrValueId destination;
  IrValueId source;
};

struct IrBinary {
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
    IrConstant constant;
    IrConvert convert;
    IrBinary binary;
    IrReturn return_;
  };
};

struct IrValue {
  Type type;
};

struct IrProgram {
  Vector<IrInstruction> instructions;
  Vector<IrValue> values;
};

const IrValue& GetIrValue(const IrProgram& program, IrValueId id);

}  // namespace fell

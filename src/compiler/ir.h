#pragma once

#include "compiler/type.h"
#include "core/string.h"
#include "core/types.h"
#include "core/vector.h"

namespace fell {

struct IrValueId {
  u32 value;
};

enum class IrOpcode {
  kConstant,
  kConvert,

  kNegate,
  kLogicalNot,

  kMultiply,
  kDivide,
  kAdd,
  kSubtract,

  kEqual,
  kNotEqual,
  kLess,
  kLessEqual,
  kGreater,
  kGreaterEqual,

  kReturn,
};

using StringConstantId = u32;

struct IrConstant {
  IrValueId destination;

  union {
    bool bool_value;
    s64 s64_value;
    u64 u64_value;
    f64 f64_value;
    StringConstantId string_value;
  };
};

struct IrConvert {
  IrValueId destination;
  IrValueId source;
};

struct IrUnary {
  IrValueId destination;
  IrValueId operand;
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
    IrUnary unary;
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
  Vector<String> string_constants;
};

const IrValue& GetIrValue(const IrProgram& program, IrValueId id);

}  // namespace fell

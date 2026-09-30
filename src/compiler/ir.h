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
  kLoadGlobal,
  kStoreGlobal,
  kLoadLocal,
  kStoreLocal,

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
using IrGlobalId = u32;
using IrLocalId = u32;

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

struct IrGlobal {
  IrValueId value;
  IrGlobalId global;
};

struct IrLocal {
  IrValueId value;
  IrLocalId local;
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
    IrGlobal global;
    IrLocal local;
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
  u32 global_count{0};
  u32 local_count{0};
};

const IrValue& GetIrValue(const IrProgram& program, IrValueId id);

}  // namespace fell

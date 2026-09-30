#pragma once

#include "compiler/type.h"
#include "core/string.h"
#include "core/type.h"
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
  kMove,
  kLabel,
  kJump,
  kJumpIfFalse,
  kCall,
  kCallNative,

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
using IrLabelId = u32;
using IrFunctionId = u32;

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

struct IrMove {
  IrValueId destination;
  IrValueId source;
};

struct IrLabel { IrLabelId label; };
struct IrJump { IrLabelId target; };
struct IrJumpIfFalse { IrValueId condition; IrLabelId target; };
struct IrCall {
  IrValueId destination;
  IrFunctionId function;
  u32 argument_offset;
  u32 argument_count;
  bool has_destination;
};
struct IrCallNative {
  IrValueId argument;
  Type argument_type;
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
    IrMove move;
    IrLabel label;
    IrJump jump;
    IrJumpIfFalse jump_if_false;
    IrCall call;
    IrCallNative call_native;
    IrUnary unary;
    IrBinary binary;
    IrReturn return_;
  };
};

struct IrValue {
  Type type;
};

struct IrFunction {
  IrFunctionId id;
  IrLabelId entry;
  Vector<IrLocalId> parameter_local_slots;
  Type return_type;
};

struct IrProgram {
  Vector<IrInstruction> instructions;
  Vector<IrValue> values;
  Vector<String> string_constants;
  Vector<IrValueId> call_arguments;
  Vector<IrFunction> functions;
  u32 main_instruction_count{0};
  u32 global_count{0};
  u32 local_count{0};
};

const IrValue& GetIrValue(const IrProgram& program, IrValueId id);

}  // namespace fell

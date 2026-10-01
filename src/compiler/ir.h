#pragma once

#include "compiler/type.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vector.h"
#include "runtime/native_function.h"

namespace fell {

inline constexpr u32 kInvalidIrValueId{static_cast<u32>(-1)};

struct IrValueId {
  u32 value{kInvalidIrValueId};
};

enum class IrOpcode {
  kInvalid,

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

using IrStringConstantId = u32;
inline constexpr IrStringConstantId kInvalidIrStringConstantId{
    static_cast<IrStringConstantId>(-1)};

using IrGlobalId = u32;
inline constexpr IrGlobalId kInvalidIrGlobalId{static_cast<IrGlobalId>(-1)};

using IrLocalId = u32;
inline constexpr IrLocalId kInvalidIrLocalId{static_cast<IrLocalId>(-1)};

using IrLabelId = u32;
inline constexpr IrLabelId kInvalidIrLabelId{static_cast<IrLabelId>(-1)};

using IrFunctionId = u32;
inline constexpr IrFunctionId kInvalidIrFunctionId{
    static_cast<IrFunctionId>(-1)};

struct IrConstant {
  IrValueId destination{IrValueId{}};

  union {
    bool bool_value{false};
    s64 s64_value;
    u64 u64_value;
    f64 f64_value;
    IrStringConstantId string_value;
  };
};

struct IrConvert {
  IrValueId destination{IrValueId{}};
  IrValueId source{IrValueId{}};
};

struct IrGlobal {
  IrValueId value{IrValueId{}};
  IrGlobalId global{kInvalidIrGlobalId};
};

struct IrLocal {
  IrValueId value{IrValueId{}};
  IrLocalId local{kInvalidIrLocalId};
};

struct IrMove {
  IrValueId destination{IrValueId{}};
  IrValueId source{IrValueId{}};
};

struct IrLabel {
  IrLabelId label{kInvalidIrLabelId};
};

struct IrJump {
  IrLabelId target{kInvalidIrLabelId};
};

struct IrJumpIfFalse {
  IrValueId condition{IrValueId{}};
  IrLabelId target{kInvalidIrLabelId};
};

struct IrCall {
  IrValueId destination{IrValueId{}};
  IrFunctionId function{kInvalidIrFunctionId};
  u32 argument_offset{0};
  u32 argument_count{0};
  bool has_destination{false};
};

struct IrCallNative {
  IrValueId argument{IrValueId{}};
  Type argument_type{Type::kInvalid};
  NativeFunctionId function{kInvalidNativeFunctionId};
};

struct IrUnary {
  IrValueId destination{IrValueId{}};
  IrValueId operand{IrValueId{}};
};

struct IrBinary {
  IrValueId destination{IrValueId{}};
  IrValueId left{IrValueId{}};
  IrValueId right{IrValueId{}};
};

struct IrReturn {
  IrValueId value{IrValueId{}};
};

struct IrInstruction {
  IrOpcode opcode{IrOpcode::kInvalid};

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
  Type type{Type::kInvalid};
};

struct IrFunction {
  IrFunctionId id;
  IrLabelId entry;
  Vector<IrLocalId> parameter_local_slots;
  Type return_type{Type::kInvalid};
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

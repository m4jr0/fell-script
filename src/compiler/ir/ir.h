#pragma once

#include "compiler/frontend/type.h"
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
};

enum class IrTerminatorKind : u8 {
  kInvalid,

  kJump,
  kBranch,
  kReturn,
  kExit,
};

using IrStringConstantId = u32;
inline constexpr IrStringConstantId kInvalidIrStringConstantId{
    static_cast<IrStringConstantId>(-1)};

using IrGlobalId = u32;
inline constexpr IrGlobalId kInvalidIrGlobalId{static_cast<IrGlobalId>(-1)};

using IrLocalId = u32;
inline constexpr IrLocalId kInvalidIrLocalId{static_cast<IrLocalId>(-1)};

using IrBlockId = u32;
inline constexpr IrBlockId kInvalidIrBlockId{static_cast<IrBlockId>(-1)};

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

struct IrJump {
  IrBlockId target{kInvalidIrBlockId};
};

struct IrBranch {
  IrValueId condition{IrValueId{}};
  IrBlockId true_target{kInvalidIrBlockId};
  IrBlockId false_target{kInvalidIrBlockId};
};

struct IrReturn {
  IrValueId value{IrValueId{}};
};

struct IrTerminator {
  IrTerminatorKind kind{IrTerminatorKind::kInvalid};

  union {
    IrJump jump;
    IrBranch branch;
    IrReturn return_;
  };
};

struct IrInstruction {
  IrOpcode opcode{IrOpcode::kInvalid};

  union {
    IrConstant constant;
    IrConvert convert;
    IrGlobal global;
    IrLocal local;
    IrMove move;
    IrCall call;
    IrCallNative call_native;
    IrUnary unary;
    IrBinary binary;
  };
};

struct IrValue {
  Type type{Type::kInvalid};
};

struct IrBasicBlock {
  Vector<IrInstruction> instructions;
  IrTerminator terminator{};
};

struct IrProcedure {
  Vector<IrBasicBlock> blocks;
  Vector<IrValue> values;
  Vector<IrValueId> call_arguments;

  IrBlockId entry{kInvalidIrBlockId};
  u32 local_count{0};
};

struct IrFunction {
  IrFunctionId id{kInvalidIrFunctionId};
  Vector<IrLocalId> parameter_local_slots;
  Type return_type{Type::kInvalid};
  IrProcedure procedure;
};

struct IrProgram {
  IrProcedure main;
  Vector<IrFunction> functions;
  Vector<String> string_constants;

  u32 global_count{0};
};

const IrValue& GetIrValue(const IrProcedure& procedure, IrValueId id);

}  // namespace fell

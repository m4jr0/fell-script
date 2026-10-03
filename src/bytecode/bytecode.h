#pragma once

#include "core/type.h"
#include "core/vector.h"
#include "runtime/native_function.h"
#include "runtime/value.h"

namespace fell {

enum class Opcode : u8 {
  kInvalid,

  kLoadImmediate,
  kLoadString,
  kLoadGlobal,
  kStoreGlobal,
  kLoadLocal,
  kStoreLocal,
  kMove,
  kJump,
  kJumpIfFalse,
  kCall,
  kCallNative,

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

  kNegateS8,
  kNegateS16,
  kNegateS32,
  kNegateS64,
  kNegateF32,
  kNegateF64,

  kLogicalNot,

  kMultiplyS8,
  kMultiplyS16,
  kMultiplyS32,
  kMultiplyS64,
  kMultiplyU8,
  kMultiplyU16,
  kMultiplyU32,
  kMultiplyU64,
  kMultiplyF32,
  kMultiplyF64,

  kDivideS8,
  kDivideS16,
  kDivideS32,
  kDivideS64,
  kDivideU8,
  kDivideU16,
  kDivideU32,
  kDivideU64,
  kDivideF32,
  kDivideF64,

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
  kAddString,

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

  kEqualBool,
  kEqualS8,
  kEqualS16,
  kEqualS32,
  kEqualS64,
  kEqualU8,
  kEqualU16,
  kEqualU32,
  kEqualU64,
  kEqualF32,
  kEqualF64,
  kEqualString,

  kNotEqualBool,
  kNotEqualS8,
  kNotEqualS16,
  kNotEqualS32,
  kNotEqualS64,
  kNotEqualU8,
  kNotEqualU16,
  kNotEqualU32,
  kNotEqualU64,
  kNotEqualF32,
  kNotEqualF64,
  kNotEqualString,

  kLessS8,
  kLessS16,
  kLessS32,
  kLessS64,
  kLessU8,
  kLessU16,
  kLessU32,
  kLessU64,
  kLessF32,
  kLessF64,

  kLessEqualS8,
  kLessEqualS16,
  kLessEqualS32,
  kLessEqualS64,
  kLessEqualU8,
  kLessEqualU16,
  kLessEqualU32,
  kLessEqualU64,
  kLessEqualF32,
  kLessEqualF64,

  kGreaterS8,
  kGreaterS16,
  kGreaterS32,
  kGreaterS64,
  kGreaterU8,
  kGreaterU16,
  kGreaterU32,
  kGreaterU64,
  kGreaterF32,
  kGreaterF64,

  kGreaterEqualS8,
  kGreaterEqualS16,
  kGreaterEqualS32,
  kGreaterEqualS64,
  kGreaterEqualU8,
  kGreaterEqualU16,
  kGreaterEqualU32,
  kGreaterEqualU64,
  kGreaterEqualF32,
  kGreaterEqualF64,

  kReturn,
};

using RegisterId = u16;
inline constexpr RegisterId kInvalidRegisterId{static_cast<RegisterId>(-1)};

using StringConstantId = u32;
inline constexpr StringConstantId kInvalidStringConstantId{
    static_cast<StringConstantId>(-1)};

using GlobalId = u32;
inline constexpr GlobalId kInvalidGlobalId{static_cast<GlobalId>(-1)};

using LocalId = u32;
inline constexpr LocalId kInvalidLocalId{static_cast<LocalId>(-1)};

using FunctionId = u32;
inline constexpr FunctionId kInvalidBytecodeFunctionId{
    static_cast<FunctionId>(-1)};

inline constexpr u32 kMaxRegisterCount{256};

struct LoadImmediateInstruction {
  RegisterId destination{kInvalidRegisterId};
  Value value;
};

struct LoadStringInstruction {
  RegisterId destination{kInvalidRegisterId};
  StringConstantId constant{kInvalidStringConstantId};
};

struct GlobalInstruction {
  RegisterId value{kInvalidRegisterId};
  GlobalId global{kInvalidGlobalId};
};

struct LocalInstruction {
  RegisterId value{kInvalidRegisterId};
  LocalId local{kInvalidLocalId};
};

struct MoveInstruction {
  RegisterId destination{kInvalidRegisterId};
  RegisterId source{kInvalidRegisterId};
};
struct JumpInstruction {
  u32 target{kMaxValue<u32>};
};
struct JumpIfFalseInstruction {
  RegisterId condition{kInvalidRegisterId};
  u32 target{kMaxValue<u32>};
};
struct CallInstruction {
  RegisterId destination{kInvalidRegisterId};
  FunctionId function{kInvalidBytecodeFunctionId};
  u32 argument_offset{0};
  u32 argument_count{0};
};
struct NativeCallInstruction {
  RegisterId argument{kInvalidRegisterId};
  ValueType type{ValueType::kInvalid};
  NativeFunctionId function{kInvalidNativeFunctionId};
};

struct ConvertInstruction {
  RegisterId destination{kInvalidRegisterId};
  RegisterId source{kInvalidRegisterId};
};

struct UnaryInstruction {
  RegisterId destination{kInvalidRegisterId};
  RegisterId operand{kInvalidRegisterId};
};

struct BinaryInstruction {
  RegisterId destination{kInvalidRegisterId};
  RegisterId left{kInvalidRegisterId};
  RegisterId right{kInvalidRegisterId};
};

struct ReturnInstruction {
  RegisterId source{kInvalidRegisterId};
  ValueType type{ValueType::kInvalid};
};

struct Instruction {
  Opcode opcode{Opcode::kInvalid};

  union {
    LoadImmediateInstruction load_immediate;
    LoadStringInstruction load_string;
    GlobalInstruction global;
    LocalInstruction local;
    MoveInstruction move;
    JumpInstruction jump;
    JumpIfFalseInstruction jump_if_false;
    CallInstruction call;
    NativeCallInstruction call_native;
    ConvertInstruction convert;
    UnaryInstruction unary;
    BinaryInstruction binary;
    ReturnInstruction return_;
  };
};

struct BytecodeFunction {
  u32 entry{kMaxValue<u32>};
  Vector<LocalId> parameter_local_slots;
  ValueType return_type{ValueType::kInvalid};
};

struct BytecodeModule {
  Vector<Instruction> instructions;
  Vector<String> string_constants;
  Vector<RegisterId> call_arguments;
  Vector<BytecodeFunction> functions;
  u32 main_instruction_count{0};
  u16 register_count{0};
  u32 global_count{0};
  u32 local_count{0};
};

}  // namespace fell
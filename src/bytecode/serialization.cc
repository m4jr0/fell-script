#include "bytecode/serialization.h"

#include <bit>
#include <type_traits>
#include <utility>

#include "core/assert.h"
#include "core/core.h"

namespace fell {
namespace {

constexpr u8 kMagic[]{'F', 'E', 'L', 'L'};

constexpr usize kHeaderSize{
    std::size(kMagic) + sizeof(u16) +  // Bytecode format version.
    sizeof(u16) +                      // Register count.
    sizeof(u32) +                      // Global count.
    sizeof(u32) +                      // Local count.
    sizeof(u32) +                      // String count.
    sizeof(u32)                        // Instruction count.
};

constexpr usize kEstimatedInstructionSize{8};

class Writer {
 public:
  explicit Writer(usize capacity) { data_.reserve(capacity); }

  void WriteU8(u8 value) { data_.push_back(value); }
  void WriteU16(u16 value) { WriteLittleEndian(value); }
  void WriteU32(u32 value) { WriteLittleEndian(value); }
  void WriteU64(u64 value) { WriteLittleEndian(value); }

  Vector<u8> Finish() { return std::move(data_); }

 private:
  template <typename T>
  void WriteLittleEndian(T value) {
    static_assert(std::is_unsigned_v<T>);

    for (usize index{0}; index < sizeof(T); ++index) {
      WriteU8(static_cast<u8>(value >> (index * 8)));
    }
  }

  Vector<u8> data_;
};

class Reader {
 public:
  explicit Reader(Span<const u8> data) : data_(data) {}

  bool ReadU8(u8& value) {
    if (position_ >= data_.size()) {
      return false;
    }

    value = data_[position_++];
    return true;
  }

  bool ReadU16(u16& value) { return ReadLittleEndian(value); }
  bool ReadU32(u32& value) { return ReadLittleEndian(value); }
  bool ReadU64(u64& value) { return ReadLittleEndian(value); }

  [[nodiscard]] usize Remaining() const { return data_.size() - position_; }
  [[nodiscard]] bool Finished() const { return position_ == data_.size(); }

 private:
  template <typename T>
  bool ReadLittleEndian(T& value) {
    static_assert(std::is_unsigned_v<T>);

    value = 0;

    for (usize index{0}; index < sizeof(T); ++index) {
      u8 byte{};

      if (!ReadU8(byte)) {
        return false;
      }

      value |= static_cast<T>(byte) << (index * 8);
    }

    return true;
  }

  Span<const u8> data_;
  usize position_{0};
};

void WriteValue(Writer& writer, const Value& value) {
  writer.WriteU8(static_cast<u8>(value.type));

  switch (value.type) {
    case ValueType::kBool:
      writer.WriteU8(value.data.bool_value ? 1 : 0);
      break;

    case ValueType::kS8:
    case ValueType::kS16:
    case ValueType::kS32:
    case ValueType::kS64:
      writer.WriteU64(std::bit_cast<u64>(value.data.s64_value));
      break;

    case ValueType::kU8:
    case ValueType::kU16:
    case ValueType::kU32:
    case ValueType::kU64:
      writer.WriteU64(value.data.u64_value);
      break;

    case ValueType::kF32:
    case ValueType::kF64:
      writer.WriteU64(std::bit_cast<u64>(value.data.f64_value));
      break;

    case ValueType::kString:
      FELL_UNREACHABLE();
  }
}

bool IsValidImmediateValueType(u8 value) {
  return value <= static_cast<u8>(ValueType::kF64);
}

bool IsValidValueType(u8 value) {
  return value <= static_cast<u8>(ValueType::kString);
}

bool ReadValue(Reader& reader, Value& value) {
  u8 type{};

  if (!reader.ReadU8(type) || !IsValidImmediateValueType(type)) {
    return false;
  }

  value.type = static_cast<ValueType>(type);

  switch (value.type) {
    case ValueType::kBool: {
      u8 data{};
      if (!reader.ReadU8(data) || data > 1) {
        return false;
      }

      value.data.bool_value = data != 0;
      break;
    }

    case ValueType::kS8:
    case ValueType::kS16:
    case ValueType::kS32:
    case ValueType::kS64: {
      u64 data{};
      if (!reader.ReadU64(data)) {
        return false;
      }

      value.data.s64_value = std::bit_cast<s64>(data);
      break;
    }

    case ValueType::kU8:
    case ValueType::kU16:
    case ValueType::kU32:
    case ValueType::kU64: {
      u64 data{};
      if (!reader.ReadU64(data)) {
        return false;
      }

      value.data.u64_value = data;
      break;
    }

    case ValueType::kString:
      return false;

    case ValueType::kF32:
    case ValueType::kF64: {
      u64 data{};
      if (!reader.ReadU64(data)) {
        return false;
      }

      value.data.f64_value = std::bit_cast<f64>(data);
      break;
    }
  }

  return true;
}

bool IsConvertOpcode(Opcode opcode) {
  return opcode >= Opcode::kConvertS8ToS16 &&
         opcode <= Opcode::kConvertF32ToF64;
}

bool IsUnaryOpcode(Opcode opcode) {
  return opcode >= Opcode::kNegateS8 && opcode <= Opcode::kLogicalNot;
}

bool IsBinaryOpcode(Opcode opcode) {
  return opcode >= Opcode::kMultiplyS8 && opcode <= Opcode::kGreaterEqualF64;
}

bool IsValidOpcode(u8 value) {
  return value <= static_cast<u8>(Opcode::kReturn);
}

bool IsValidRegister(RegisterId id, const BytecodeModule& module) {
  return id < module.register_count;
}

}  // namespace

Vector<u8> SerializeBytecode(const BytecodeModule& module) {
  FELL_ASSERT(module.instructions.size() <= kMaxValue<u32>);
  FELL_ASSERT(module.string_constants.size() <= kMaxValue<u32>);

  Writer writer{
      kHeaderSize + module.instructions.size() * kEstimatedInstructionSize,
  };

  for (const u8 byte : kMagic) {
    writer.WriteU8(byte);
  }

  writer.WriteU16(kBytecodeFormatVersion);
  writer.WriteU16(module.register_count);
  writer.WriteU32(module.global_count);
  writer.WriteU32(module.local_count);
  writer.WriteU32(static_cast<u32>(module.string_constants.size()));
  writer.WriteU32(static_cast<u32>(module.instructions.size()));

  for (const String& string : module.string_constants) {
    FELL_ASSERT(string.size() <= kMaxValue<u32>);
    writer.WriteU32(static_cast<u32>(string.size()));
    for (const char character : string) {
      writer.WriteU8(static_cast<u8>(character));
    }
  }

  for (const Instruction& instruction : module.instructions) {
    writer.WriteU8(static_cast<u8>(instruction.opcode));

    if (instruction.opcode == Opcode::kLoadImmediate) {
      writer.WriteU16(instruction.load_immediate.destination);
      WriteValue(writer, instruction.load_immediate.value);
    } else if (instruction.opcode == Opcode::kLoadString) {
      writer.WriteU16(instruction.load_string.destination);
      writer.WriteU32(instruction.load_string.constant);
    } else if (instruction.opcode == Opcode::kLoadGlobal ||
               instruction.opcode == Opcode::kStoreGlobal) {
      writer.WriteU16(instruction.global.value);
      writer.WriteU32(instruction.global.global);
    } else if (instruction.opcode == Opcode::kLoadLocal ||
               instruction.opcode == Opcode::kStoreLocal) {
      writer.WriteU16(instruction.local.value);
      writer.WriteU32(instruction.local.local);
    } else if (instruction.opcode == Opcode::kMove) {
      writer.WriteU16(instruction.move.destination);
      writer.WriteU16(instruction.move.source);
    } else if (instruction.opcode == Opcode::kJump) {
      writer.WriteU32(instruction.jump.target);
    } else if (instruction.opcode == Opcode::kJumpIfFalse) {
      writer.WriteU16(instruction.jump_if_false.condition);
      writer.WriteU32(instruction.jump_if_false.target);
    } else if (IsConvertOpcode(instruction.opcode)) {
      writer.WriteU16(instruction.convert.destination);
      writer.WriteU16(instruction.convert.source);
    } else if (IsUnaryOpcode(instruction.opcode)) {
      writer.WriteU16(instruction.unary.destination);
      writer.WriteU16(instruction.unary.operand);
    } else if (IsBinaryOpcode(instruction.opcode)) {
      writer.WriteU16(instruction.binary.destination);
      writer.WriteU16(instruction.binary.left);
      writer.WriteU16(instruction.binary.right);
    } else {
      FELL_ASSERT(instruction.opcode == Opcode::kReturn);
      writer.WriteU16(instruction.return_.source);
      writer.WriteU8(static_cast<u8>(instruction.return_.type));
    }
  }

  return writer.Finish();
}

BytecodeReadResult DeserializeBytecode(Span<const u8> data) {
  Reader reader{data};

  for (const u8 expected : kMagic) {
    u8 actual{};
    if (!reader.ReadU8(actual) || actual != expected) {
      return {.module = {}, .error = "invalid Fell bytecode magic"};
    }
  }

  u16 version{};
  if (!reader.ReadU16(version)) {
    return {.module = {}, .error = "truncated Fell bytecode header"};
  }

  if (version != kBytecodeFormatVersion) {
    return {.module = {}, .error = "unsupported Fell bytecode version"};
  }

  BytecodeModule module{
      .instructions = {},
      .string_constants = {},
      .register_count = 0,
      .global_count = 0,
      .local_count = 0,
  };

  if (!reader.ReadU16(module.register_count) ||
      module.register_count > kMaxRegisterCount) {
    return {.module = {}, .error = "invalid Fell register count"};
  }

  if (!reader.ReadU32(module.global_count) ||
      !reader.ReadU32(module.local_count)) {
    return {.module = {}, .error = "truncated Fell bytecode header"};
  }

  u32 string_count{};
  if (!reader.ReadU32(string_count)) {
    return {.module = {}, .error = "truncated Fell bytecode header"};
  }

  u32 instruction_count{};
  if (!reader.ReadU32(instruction_count)) {
    return {.module = {}, .error = "truncated Fell bytecode header"};
  }

  module.string_constants.reserve(string_count);
  for (u32 index{0}; index < string_count; ++index) {
    u32 length{};
    if (!reader.ReadU32(length) || length > reader.Remaining()) {
      return {.module = {}, .error = "invalid Fell string constant"};
    }

    String string;
    string.reserve(length);
    for (u32 byte_index{0}; byte_index < length; ++byte_index) {
      u8 byte{};
      if (!reader.ReadU8(byte)) {
        return {.module = {}, .error = "truncated Fell string constant"};
      }
      string.push_back(static_cast<char>(byte));
    }
    module.string_constants.push_back(std::move(string));
  }

  if (instruction_count > reader.Remaining()) {
    return {
        .module = {},
        .error = "invalid Fell instruction count",
    };
  }

  module.instructions.reserve(instruction_count);

  for (u32 index{0}; index < instruction_count; ++index) {
    u8 raw_opcode{};
    if (!reader.ReadU8(raw_opcode) || !IsValidOpcode(raw_opcode)) {
      return {.module = {}, .error = "invalid Fell bytecode opcode"};
    }

    const auto opcode{static_cast<Opcode>(raw_opcode)};

    if (opcode == Opcode::kLoadGlobal || opcode == Opcode::kStoreGlobal) {
      GlobalInstruction global{};
      if (!reader.ReadU16(global.value) || !reader.ReadU32(global.global)) {
        return {.module = {}, .error = "truncated global instruction"};
      }
      if (!IsValidRegister(global.value, module) ||
          global.global >= module.global_count) {
        return {.module = {}, .error = "invalid global instruction"};
      }
      module.instructions.push_back({.opcode = opcode, .global = global});
    } else if (opcode == Opcode::kLoadLocal || opcode == Opcode::kStoreLocal) {
      LocalInstruction local{};
      if (!reader.ReadU16(local.value) || !reader.ReadU32(local.local)) {
        return {.module = {}, .error = "truncated local instruction"};
      }
      if (!IsValidRegister(local.value, module) ||
          local.local >= module.local_count) {
        return {.module = {}, .error = "invalid local instruction"};
      }
      module.instructions.push_back({.opcode = opcode, .local = local});
    } else if (opcode == Opcode::kLoadString) {
      LoadStringInstruction load{};
      if (!reader.ReadU16(load.destination) || !reader.ReadU32(load.constant)) {
        return {.module = {}, .error = "truncated string load instruction"};
      }
      if (!IsValidRegister(load.destination, module) ||
          load.constant >= module.string_constants.size()) {
        return {.module = {}, .error = "invalid string load instruction"};
      }
      module.instructions.push_back({.opcode = opcode, .load_string = load});
    } else if (opcode == Opcode::kLoadImmediate) {
      LoadImmediateInstruction load{};
      if (!reader.ReadU16(load.destination) || !ReadValue(reader, load.value)) {
        return {.module = {}, .error = "truncated load instruction"};
      }

      if (!IsValidRegister(load.destination, module)) {
        return {.module = {}, .error = "invalid register in load instruction"};
      }

      module.instructions.push_back({
          .opcode = opcode,
          .load_immediate = load,
      });
    } else if (opcode == Opcode::kMove) {
      MoveInstruction move{};
      if (!reader.ReadU16(move.destination) || !reader.ReadU16(move.source) ||
          !IsValidRegister(move.destination, module) ||
          !IsValidRegister(move.source, module))
        return {.module = {}, .error = "invalid move instruction"};
      module.instructions.push_back({.opcode = opcode, .move = move});
    } else if (opcode == Opcode::kJump) {
      JumpInstruction jump{};
      if (!reader.ReadU32(jump.target) || jump.target > instruction_count)
        return {.module = {}, .error = "invalid jump instruction"};
      module.instructions.push_back({.opcode = opcode, .jump = jump});
    } else if (opcode == Opcode::kJumpIfFalse) {
      JumpIfFalseInstruction jump{};
      if (!reader.ReadU16(jump.condition) || !reader.ReadU32(jump.target) ||
          !IsValidRegister(jump.condition, module) ||
          jump.target > instruction_count)
        return {.module = {}, .error = "invalid conditional jump instruction"};
      module.instructions.push_back({.opcode = opcode, .jump_if_false = jump});
    } else if (IsConvertOpcode(opcode)) {
      ConvertInstruction convert{};
      if (!reader.ReadU16(convert.destination) ||
          !reader.ReadU16(convert.source)) {
        return {.module = {}, .error = "truncated convert instruction"};
      }

      if (!IsValidRegister(convert.destination, module) ||
          !IsValidRegister(convert.source, module)) {
        return {
            .module = {},
            .error = "invalid register in convert instruction",
        };
      }

      module.instructions.push_back({
          .opcode = opcode,
          .convert = convert,
      });
    } else if (IsUnaryOpcode(opcode)) {
      UnaryInstruction unary{};
      if (!reader.ReadU16(unary.destination) ||
          !reader.ReadU16(unary.operand)) {
        return {.module = {}, .error = "truncated unary instruction"};
      }

      if (!IsValidRegister(unary.destination, module) ||
          !IsValidRegister(unary.operand, module)) {
        return {
            .module = {},
            .error = "invalid register in unary instruction",
        };
      }

      module.instructions.push_back({
          .opcode = opcode,
          .unary = unary,
      });
    } else if (IsBinaryOpcode(opcode)) {
      BinaryInstruction binary{};
      if (!reader.ReadU16(binary.destination) || !reader.ReadU16(binary.left) ||
          !reader.ReadU16(binary.right)) {
        return {.module = {}, .error = "truncated binary instruction"};
      }

      if (!IsValidRegister(binary.destination, module) ||
          !IsValidRegister(binary.left, module) ||
          !IsValidRegister(binary.right, module)) {
        return {
            .module = {},
            .error = "invalid register in binary instruction",
        };
      }

      module.instructions.push_back({
          .opcode = opcode,
          .binary = binary,
      });
    } else {
      ReturnInstruction return_instruction{};
      u8 raw_type{};

      if (!reader.ReadU16(return_instruction.source) ||
          !reader.ReadU8(raw_type) || !IsValidValueType(raw_type)) {
        return {.module = {}, .error = "invalid return instruction"};
      }

      return_instruction.type = static_cast<ValueType>(raw_type);

      if (!IsValidRegister(return_instruction.source, module)) {
        return {
            .module = {},
            .error = "invalid register in return instruction",
        };
      }

      module.instructions.push_back({
          .opcode = opcode,
          .return_ = return_instruction,
      });
    }
  }

  if (!reader.Finished()) {
    return {.module = {}, .error = "trailing data in Fell bytecode"};
  }

  return {
      .module = std::move(module),
      .error = {},
  };
}

}  // namespace fell

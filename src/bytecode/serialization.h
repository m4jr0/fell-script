#pragma once

#include "bytecode/bytecode.h"
#include "core/span.h"
#include "core/string.h"
#include "core/vector.h"

namespace fell {

inline constexpr u16 kBytecodeFormatVersion{1};

struct BytecodeReadResult {
  BytecodeModule module;
  String error;

  [[nodiscard]] bool Succeeded() const { return error.empty(); }
};

Vector<u8> SerializeBytecode(const BytecodeModule& module);
BytecodeReadResult DeserializeBytecode(Span<const u8> data);

}  // namespace fell

#pragma once

#include "bytecode/bytecode.h"
#include "core/string.h"

namespace fell {

StringView ToString(Opcode opcode);
String DumpBytecode(const BytecodeModule& module);

}  // namespace fell

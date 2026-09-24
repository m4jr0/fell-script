#pragma once

#include "bytecode/bytecode.h"
#include "core/types.h"

namespace fell {

class Vm {
 public:
  s32 Execute(const BytecodeModule& program);

 private:
  s32 registers_[kMaxRegisterCount]{};
};

}  // namespace fell

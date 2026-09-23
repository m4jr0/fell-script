#pragma once

#include "bytecode/bytecode.h"
#include "core/types.h"

namespace fell {

class Vm {
 public:
  s32 Execute(const Program& program);

 private:
  static constexpr u16 kMaxRegisterCount = 256;

  s32 registers_[kMaxRegisterCount] = {};
};

}  // namespace fell
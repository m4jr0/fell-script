#pragma once

#include "bytecode/bytecode.h"

namespace fell {

class Vm {
 public:
  Value Execute(const BytecodeModule& program);

 private:
  ValueData registers_[kMaxRegisterCount]{};
};

}  // namespace fell

#pragma once

#include <optional>

#include "bytecode/bytecode.h"

namespace fell {

class Vm {
 public:
  std::optional<Value> Execute(const BytecodeModule& program);

 private:
  ValueData registers_[kMaxRegisterCount]{};
};

}  // namespace fell

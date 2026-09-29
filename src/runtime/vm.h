#pragma once

#include <optional>

#include "bytecode/bytecode.h"
#include "core/memory.h"
#include "core/vector.h"

namespace fell {

class Vm {
 public:
  std::optional<Value> Execute(const BytecodeModule& program);

 private:
  ValueData registers_[kMaxRegisterCount]{};
  Vector<UniquePtr<RuntimeString>> strings_;
  Vector<ValueData> globals_;
};

}  // namespace fell

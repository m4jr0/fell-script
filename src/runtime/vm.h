#pragma once

#include <array>
#include <optional>

#include "bytecode/bytecode.h"
#include "core/memory.h"
#include "core/vector.h"

namespace fell {

class Vm {
 public:
  std::optional<Value> Execute(const BytecodeModule& program);

 private:
  struct Frame {
    std::array<ValueData, kMaxRegisterCount> registers{};
    Vector<ValueData> locals;
    s64 return_ip{-1};
    RegisterId return_destination{0};
  };

  Vector<UniquePtr<RuntimeString>> strings_;
  Vector<ValueData> globals_;
  Vector<Frame> frames_;
};

}  // namespace fell

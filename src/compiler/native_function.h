#pragma once

#include "compiler/frontend/type.h"
#include "core/string.h"
#include "core/vector.h"
#include "runtime/native_function.h"

namespace fell {

struct NativeFunctionDescriptor {
  StringView name;
  Vector<Type> parameter_types;
  Type return_type{Type::kInvalid};
  NativeFunctionId id{kInvalidNativeFunctionId};
  bool accepts_any_value{false};
};

inline const Vector<NativeFunctionDescriptor>& GetNativeFunctionDescriptors() {
  static const Vector<NativeFunctionDescriptor> descriptors{
      {
          .name = "print",
          .parameter_types = {Type::kInvalid},
          .return_type = Type::kUnit,
          .id = kPrintNativeFunctionId,
          .accepts_any_value = true,
      },
      {
          .name = "assert",
          .parameter_types = {Type::kBool},
          .return_type = Type::kUnit,
          .id = kAssertNativeFunctionId,
      },
  };

  return descriptors;
}

}  // namespace fell

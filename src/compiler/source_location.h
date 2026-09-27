#pragma once

#include "core/core.h"
#include "core/types.h"

namespace fell {

struct SourceLocation {
  usize offset{0};
  u32 line{1};
  u32 column{1};
};

struct SourceSpan {
  SourceLocation start{};
  usize length{0};
};

}  // namespace fell

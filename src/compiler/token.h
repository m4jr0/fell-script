#pragma once

#include "core/types.h"

namespace fell {

enum class TokenType {
  kInteger,
  kEndOfFile,
  kInvalid,
};

struct Token {
  TokenType type;
  s32 value;
};

}  // namespace fell
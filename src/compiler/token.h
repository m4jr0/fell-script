#pragma once

#include "core/types.h"

namespace fell {

enum class TokenType {
  kIntegerLiteral,
  kPlus,
  kMinus,
  kSemicolon,
  kEndOfFile,
  kInvalid,

  kCount,
};

struct Token {
  TokenType type{TokenType::kInvalid};
  s32 value{0};
};

}  // namespace fell

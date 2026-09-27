#pragma once

#include "core/string.h"
#include "core/types.h"

namespace fell {

enum class TokenType {
  kIntegerLiteral,
  kFloatLiteral,

  kLeftParen,
  kRightParen,

  kStar,
  kSlash,
  kPlus,
  kMinus,
  kSemicolon,
  kEndOfFile,
  kInvalid,

  kCount,
};

struct Token {
  TokenType type{TokenType::kInvalid};
  StringView lexeme{};
};

StringView ToString(TokenType type);

}  // namespace fell

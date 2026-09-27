#pragma once

#include "core/string.h"
#include "core/types.h"

namespace fell {

enum class TokenType {
  kTrue,
  kFalse,

  kIntegerLiteral,
  kFloatLiteral,

  kIdentifier,

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

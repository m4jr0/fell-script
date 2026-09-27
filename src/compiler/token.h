#pragma once

#include "compiler/source_location.h"
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
  SourceSpan span{};
};

StringView ToString(TokenType type);

}  // namespace fell

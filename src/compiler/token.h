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
  kStringLiteral,

  kIdentifier,

  kLeftParen,
  kRightParen,

  kStar,
  kSlash,
  kPlus,
  kMinus,

  kBang,
  kBangEqual,
  kEqualEqual,
  kLess,
  kLessEqual,
  kGreater,
  kGreaterEqual,

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

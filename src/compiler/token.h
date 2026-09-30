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
  kLet,
  kMut,
  kIf,
  kElse,
  kWhile,
  kBreak,
  kContinue,
  kFor,
  kSwitch,
  kCase,
  kDefault,

  kLeftParen,
  kRightParen,
  kLeftBrace,
  kRightBrace,

  kStar,
  kSlash,
  kPlus,
  kMinus,
  kAmpAmp,
  kPipePipe,

  kBang,
  kBangEqual,
  kEqual,
  kEqualEqual,
  kLess,
  kLessEqual,
  kGreater,
  kGreaterEqual,

  kColon,
  kSemicolon,
  kQuestion,

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

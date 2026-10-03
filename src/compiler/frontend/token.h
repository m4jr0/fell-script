#pragma once

#include "compiler/frontend/source_location.h"
#include "core/string.h"
#include "core/type.h"

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
  kFn,
  kReturn,
  kSwitch,
  kCase,
  kDefault,

  kLeftParen,
  kRightParen,
  kComma,
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

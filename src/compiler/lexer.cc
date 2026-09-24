#include "compiler/lexer.h"

#include "core/char.h"
#include "core/string.h"

namespace fell {

Lexer::Lexer(StringView source) : source_(source) {}

Token Lexer::NextToken() {
  while (position_ < source_.size() && IsWhitespace(source_[position_])) {
    ++position_;
  }

  if (position_ == source_.size()) {
    return {TokenType::kEndOfFile, 0};
  }

  const char character{source_[position_]};

  switch (character) {
    case '+':
      ++position_;
      return {TokenType::kPlus, 0};

    case '-':
      ++position_;
      return {TokenType::kMinus, 0};

    case ';':
      ++position_;
      return {TokenType::kSemicolon, 0};
  }

  if (IsDigit(character)) {
    return TokenizeIntegerLiteral();
  }

  ++position_;
  return {TokenType::kInvalid, 0};
}

Token Lexer::TokenizeIntegerLiteral() {
  s32 value{0};

  while (position_ < source_.size() && IsDigit(source_[position_])) {
    value = value * 10 + (source_[position_] - '0');
    ++position_;
  }

  return {TokenType::kIntegerLiteral, value};
}

}  // namespace fell

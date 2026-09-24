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
    return MakeToken(TokenType::kPlus, 1);
  }

  const char character{source_[position_]};

  switch (character) {
    case '+':
      ++position_;
      return MakeToken(TokenType::kPlus, 1);

    case '-':
      ++position_;
      return MakeToken(TokenType::kMinus, 1);

    case ';':
      ++position_;
      return MakeToken(TokenType::kSemicolon, 1);
  }

  if (IsDigit(character)) {
    return TokenizeNumber();
  }

  ++position_;
  return {TokenType::kInvalid, 0};
}

bool Lexer::Consume(StringView text) {
  if (source_.substr(position_, text.size()) != text) {
    return false;
  }

  position_ += text.size();
  return true;
}

Token Lexer::MakeToken(TokenType type, usize start) const {
  return {
      .type = type,
      .lexeme = source_.substr(start, position_ - start),
  };
}

Token Lexer::TokenizeNumber() {
  const auto start{position_};

  while (position_ < source_.size() && IsDigit(source_[position_])) {
    ++position_;
  }

  auto type{TokenType::kIntegerLiteral};

  if (position_ + 1 < source_.size() && source_[position_] == '.' &&
      IsDigit(source_[position_ + 1])) {
    type = TokenType::kFloatLiteral;
    ++position_;

    while (position_ < source_.size() && IsDigit(source_[position_])) {
      ++position_;
    }
  }

  if (type == TokenType::kIntegerLiteral) {
    if (Consume("f32") || Consume("f64") || Consume("f")) {
      type = TokenType::kFloatLiteral;
    } else if (!Consume("s8") && !Consume("s16") && !Consume("s32") &&
               !Consume("s64") && !Consume("u8") && !Consume("u16") &&
               !Consume("u32")) {
      Consume("u64");
    }
  } else {
    if (!Consume("f32") && !Consume("f64")) {
      Consume("f");
    }
  }

  return MakeToken(type, start);
}

}  // namespace fell

#include "compiler/lexer.h"

#include "core/char.h"
#include "core/string.h"

namespace fell {

Lexer::Lexer(StringView source) : source_(source) {}

Token Lexer::NextToken() {
  SkipWhitespace();

  const SourceLocation start{
      .offset = position_,
      .line = line_,
      .column = column_,
  };

  if (position_ == source_.size()) {
    return MakeToken(TokenType::kEndOfFile, start);
  }

  const char character{source_[position_]};

  switch (character) {
    case '(':
      Advance();
      return MakeToken(TokenType::kLeftParen, start);

    case ')':
      Advance();
      return MakeToken(TokenType::kRightParen, start);

    case '*':
      Advance();
      return MakeToken(TokenType::kStar, start);

    case '/':
      Advance();
      return MakeToken(TokenType::kSlash, start);

    case '+':
      Advance();
      return MakeToken(TokenType::kPlus, start);

    case '-':
      Advance();
      return MakeToken(TokenType::kMinus, start);

    case ';':
      Advance();
      return MakeToken(TokenType::kSemicolon, start);
  }

  if (IsIdentifierStart(character)) {
    return TokenizeIdentifier(start);
  }

  if (IsDigit(character)) {
    return TokenizeNumber(start);
  }

  Advance();
  return MakeToken(TokenType::kInvalid, start);
}

char Lexer::Advance() {
  const char character{source_[position_++]};

  if (character == '\n') {
    ++line_;
    column_ = 1;
  } else {
    ++column_;
  }

  return character;
}

bool Lexer::Consume(StringView text) {
  if (source_.substr(position_, text.size()) != text) {
    return false;
  }

  for (usize index{0}; index < text.size(); ++index) {
    Advance();
  }

  return true;
}

void Lexer::SkipWhitespace() {
  while (position_ < source_.size() && IsWhitespace(source_[position_])) {
    Advance();
  }
}

Token Lexer::MakeToken(TokenType type, SourceLocation start) const {
  return {
      .type = type,
      .lexeme = source_.substr(start.offset, position_ - start.offset),
      .span =
          {
              .start = start,
              .length = position_ - start.offset,
          },
  };
}

Token Lexer::TokenizeNumber(SourceLocation start) {
  while (position_ < source_.size() && IsDigit(source_[position_])) {
    Advance();
  }

  auto type{TokenType::kIntegerLiteral};

  if (position_ + 1 < source_.size() && source_[position_] == '.' &&
      IsDigit(source_[position_ + 1])) {
    type = TokenType::kFloatLiteral;
    Advance();

    while (position_ < source_.size() && IsDigit(source_[position_])) {
      Advance();
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

Token Lexer::TokenizeIdentifier(SourceLocation start) {
  while (position_ < source_.size() &&
         IsIdentifierContinue(source_[position_])) {
    Advance();
  }

  const StringView lexeme{
      source_.substr(start.offset, position_ - start.offset),
  };

  return MakeToken(GetIdentifierType(lexeme), start);
}

bool Lexer::IsIdentifierStart(char character) const {
  return IsAlpha(character) || character == '_';
}

bool Lexer::IsIdentifierContinue(char character) const {
  return IsIdentifierStart(character) || IsDigit(character);
}

TokenType Lexer::GetIdentifierType(StringView lexeme) const {
  switch (lexeme[0]) {
    case 'f':
      if (lexeme == "false") {
        return TokenType::kFalse;
      }
      break;

    case 't':
      if (lexeme == "true") {
        return TokenType::kTrue;
      }
      break;
  }

  return TokenType::kIdentifier;
}

}  // namespace fell

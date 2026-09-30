#include "compiler/lexer.h"

#include "core/char.h"
#include "core/string.h"

namespace fell {

Lexer::Lexer(StringView source) : source_(source) {}

Token Lexer::NextToken() {
  SourceLocation error_start{};
  if (!SkipTrivia(error_start)) {
    return MakeToken(TokenType::kInvalid, error_start);
  }

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

    case ',':
      Advance();
      return MakeToken(TokenType::kComma, start);

    case '{':
      Advance();
      return MakeToken(TokenType::kLeftBrace, start);

    case '}':
      Advance();
      return MakeToken(TokenType::kRightBrace, start);

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

    case '&':
      Advance();
      return MakeToken(Match('&') ? TokenType::kAmpAmp : TokenType::kInvalid, start);

    case '|':
      Advance();
      return MakeToken(Match('|') ? TokenType::kPipePipe : TokenType::kInvalid, start);

    case '!':
      Advance();
      return MakeToken(Match('=') ? TokenType::kBangEqual : TokenType::kBang,
                       start);

    case '=':
      Advance();
      return MakeToken(Match('=') ? TokenType::kEqualEqual : TokenType::kEqual,
                       start);

    case '<':
      Advance();
      return MakeToken(Match('=') ? TokenType::kLessEqual : TokenType::kLess,
                       start);

    case '>':
      Advance();
      return MakeToken(
          Match('=') ? TokenType::kGreaterEqual : TokenType::kGreater, start);

    case ':':
      Advance();
      return MakeToken(TokenType::kColon, start);

    case ';':
      Advance();
      return MakeToken(TokenType::kSemicolon, start);

    case '?':
      Advance();
      return MakeToken(TokenType::kQuestion, start);

    case '"':
      return TokenizeString(start);
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

char Lexer::Peek() const {
  return position_ < source_.size() ? source_[position_] : '\0';
}

char Lexer::PeekNext() const {
  return position_ + 1 < source_.size() ? source_[position_ + 1] : '\0';
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

bool Lexer::Match(char expected) {
  if (position_ >= source_.size() || source_[position_] != expected) {
    return false;
  }

  Advance();
  return true;
}

bool Lexer::SkipTrivia(SourceLocation& error_start) {
  while (position_ < source_.size()) {
    if (IsWhitespace(Peek())) {
      Advance();
      continue;
    }

    if (Peek() != '/') {
      return true;
    }

    if (PeekNext() == '/') {
      SkipLineComment();
      continue;
    }

    if (PeekNext() == '*') {
      error_start = {
          .offset = position_,
          .line = line_,
          .column = column_,
      };

      if (!SkipBlockComment()) {
        return false;
      }

      continue;
    }

    return true;
  }

  return true;
}

void Lexer::SkipLineComment() {
  Advance();
  Advance();

  while (position_ < source_.size() && Peek() != '\n') {
    Advance();
  }
}

bool Lexer::SkipBlockComment() {
  Advance();
  Advance();

  while (position_ < source_.size()) {
    if (Peek() == '*' && PeekNext() == '/') {
      Advance();
      Advance();
      return true;
    }

    Advance();
  }

  return false;
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

Token Lexer::TokenizeString(SourceLocation start) {
  Advance();

  while (position_ < source_.size() && Peek() != '"') {
    if (Peek() == '\n') {
      return MakeToken(TokenType::kInvalid, start);
    }
    Advance();
  }

  if (position_ == source_.size()) {
    return MakeToken(TokenType::kInvalid, start);
  }

  Advance();
  return MakeToken(TokenType::kStringLiteral, start);
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
    case 'b':
      if (lexeme == "break") return TokenType::kBreak;
      break;

    case 'c':
      if (lexeme == "case") return TokenType::kCase;
      if (lexeme == "continue") return TokenType::kContinue;
      break;

    case 'd':
      if (lexeme == "default") return TokenType::kDefault;
      break;

    case 'e':
      if (lexeme == "else") return TokenType::kElse;
      break;

    case 'f':
      if (lexeme == "false") return TokenType::kFalse;
      if (lexeme == "for") return TokenType::kFor;
      if (lexeme == "fn") return TokenType::kFn;
      break;

    case 'i':
      if (lexeme == "if") return TokenType::kIf;
      break;

    case 'l':
      if (lexeme == "let") {
        return TokenType::kLet;
      }
      break;

    case 'm':
      if (lexeme == "mut") {
        return TokenType::kMut;
      }
      break;

    case 'r':
      if (lexeme == "return") return TokenType::kReturn;
      break;

    case 's':
      if (lexeme == "switch") return TokenType::kSwitch;
      break;

    case 't':
      if (lexeme == "true") {
        return TokenType::kTrue;
      }
      break;

    case 'w':
      if (lexeme == "while") return TokenType::kWhile;
      break;
  }

  return TokenType::kIdentifier;
}

}  // namespace fell

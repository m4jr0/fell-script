#pragma once

#include "compiler/frontend/source_location.h"
#include "compiler/frontend/token.h"
#include "core/string.h"
#include "core/type.h"

namespace fell {

class Lexer {
 public:
  explicit Lexer(StringView source);

  Token NextToken();

 private:
  char Advance();
  [[nodiscard]] char Peek() const;
  [[nodiscard]] char PeekNext() const;
  bool Consume(StringView text);
  bool Match(char expected);

  bool SkipTrivia(SourceLocation& error_start);
  void SkipLineComment();
  bool SkipBlockComment();

  [[nodiscard]] Token MakeToken(TokenType type, SourceLocation start) const;

  Token TokenizeNumber(SourceLocation start);
  Token TokenizeString(SourceLocation start);
  Token TokenizeIdentifier(SourceLocation start);

  [[nodiscard]] bool IsIdentifierStart(char character) const;
  [[nodiscard]] bool IsIdentifierContinue(char character) const;
  [[nodiscard]] TokenType CheckKeyword(StringView lexeme, usize start,
                                       StringView rest, TokenType type) const;
  [[nodiscard]] TokenType GetIdentifierType(StringView lexeme) const;

  StringView source_;
  usize position_{0};
  u32 line_{1};
  u32 column_{1};
};

}  // namespace fell

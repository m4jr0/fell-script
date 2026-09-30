#pragma once

#include "compiler/source_location.h"
#include "compiler/token.h"
#include "core/type.h"
#include "core/string.h"

namespace fell {

class Lexer {
 public:
  explicit Lexer(StringView source);

  Token NextToken();

 private:
  char Advance();
  char Peek() const;
  char PeekNext() const;
  bool Consume(StringView text);
  bool Match(char expected);

  bool SkipTrivia(SourceLocation& error_start);
  void SkipLineComment();
  bool SkipBlockComment();

  Token MakeToken(TokenType type, SourceLocation start) const;

  Token TokenizeNumber(SourceLocation start);
  Token TokenizeString(SourceLocation start);
  Token TokenizeIdentifier(SourceLocation start);

  bool IsIdentifierStart(char character) const;
  bool IsIdentifierContinue(char character) const;
  TokenType GetIdentifierType(StringView lexeme) const;

  StringView source_;
  usize position_{0};
  u32 line_{1};
  u32 column_{1};
};

}  // namespace fell

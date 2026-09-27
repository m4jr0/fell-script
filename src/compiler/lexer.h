#pragma once

#include "compiler/token.h"
#include "core/core.h"
#include "core/string.h"

namespace fell {

class Lexer {
 public:
  explicit Lexer(StringView source);

  Token NextToken();

 private:
  bool Consume(StringView text);

  Token MakeToken(TokenType type, usize start) const;

  Token TokenizeNumber();
  Token TokenizeIdentifier();

  bool IsIdentifierStart(char character) const;
  bool IsIdentifierContinue(char character) const;
  TokenType GetIdentifierType(StringView lexeme) const;

  StringView source_;
  usize position_{0};
};

}  // namespace fell

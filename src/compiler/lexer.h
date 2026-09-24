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

  StringView source_;
  usize position_{0};
};

}  // namespace fell

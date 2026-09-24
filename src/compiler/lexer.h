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
  Token TokenizeIntegerLiteral();

  StringView source_;
  usize position_{0};
};

}  // namespace fell

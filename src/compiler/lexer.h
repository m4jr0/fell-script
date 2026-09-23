#pragma once

#include "compiler/token.h"
#include "core/string.h"

namespace fell {

class Lexer {
 public:
  explicit Lexer(StringView source);

  Token NextToken();

 private:
  StringView source_;
  std::size_t position_ = 0;
};

}  // namespace fell
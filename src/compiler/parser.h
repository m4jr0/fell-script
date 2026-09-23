#pragma once

#include "compiler/ast.h"
#include "compiler/lexer.h"

namespace fell {

class Parser {
 public:
  explicit Parser(Lexer& lexer);

  bool ParseExpression(IntegerExpression& expression);

 private:
  Lexer& lexer_;
};

}  // namespace fell
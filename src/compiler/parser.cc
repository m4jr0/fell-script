#include "compiler/parser.h"

namespace fell {

Parser::Parser(Lexer& lexer) : lexer_(lexer) {}

bool Parser::ParseExpression(IntegerExpression& expression) {
  const Token token = lexer_.NextToken();

  if (token.type != TokenType::kInteger) {
    return false;
  }

  expression.value = token.value;

  const Token end = lexer_.NextToken();

  if (end.type != TokenType::kEndOfFile) {
    return false;
  }

  return true;
}

}  // namespace fell
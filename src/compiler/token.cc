#include "compiler/token.h"

#include "core/assert.h"

namespace fell {

StringView ToString(TokenType type) {
  switch (type) {
    case TokenType::kIntegerLiteral:
      return "integer_literal";
    case TokenType::kFloatLiteral:
      return "float_literal";
    case TokenType::kLeftParen:
      return "left_paren";
    case TokenType::kRightParen:
      return "right_paren";
    case TokenType::kStar:
      return "star";
    case TokenType::kSlash:
      return "slash";
    case TokenType::kPlus:
      return "plus";
    case TokenType::kMinus:
      return "minus";
    case TokenType::kSemicolon:
      return "semicolon";
    case TokenType::kEndOfFile:
      return "eof";
    case TokenType::kInvalid:
      return "invalid";
    case TokenType::kCount:
      FELL_UNREACHABLE();
  }

  FELL_UNREACHABLE();
}

}  // namespace fell

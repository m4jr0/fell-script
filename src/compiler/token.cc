#include "compiler/token.h"

#include "core/assert.h"

namespace fell {

StringView ToString(TokenType type) {
  switch (type) {
    case TokenType::kTrue:
      return "true";
    case TokenType::kFalse:
      return "false";
    case TokenType::kIntegerLiteral:
      return "integer_literal";
    case TokenType::kFloatLiteral:
      return "float_literal";
    case TokenType::kIdentifier:
      return "identifier";
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
    case TokenType::kBang:
      return "bang";
    case TokenType::kBangEqual:
      return "bang_equal";
    case TokenType::kEqualEqual:
      return "equal_equal";
    case TokenType::kLess:
      return "less";
    case TokenType::kLessEqual:
      return "less_equal";
    case TokenType::kGreater:
      return "greater";
    case TokenType::kGreaterEqual:
      return "greater_equal";
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

#include "compiler/parser.h"

#include <charconv>

#include "core/assert.h"
#include "core/core.h"

namespace fell {
namespace {

UnaryOperator GetUnaryOperator(TokenType type) {
  switch (type) {
    case TokenType::kMinus:
      return UnaryOperator::kNegate;
    default:
      FELL_UNREACHABLE();
  }
}

BinaryOperator GetBinaryOperator(TokenType type) {
  switch (type) {
    case TokenType::kStar:
      return BinaryOperator::kMultiply;
    case TokenType::kSlash:
      return BinaryOperator::kDivide;
    case TokenType::kPlus:
      return BinaryOperator::kAdd;
    case TokenType::kMinus:
      return BinaryOperator::kSubtract;
    default:
      FELL_UNREACHABLE();
  }
}

}  // namespace

Parser::Parser(Lexer& lexer, Ast& ast) : lexer_(lexer), ast_(ast) { Advance(); }

bool Parser::ParseCompilationUnit(CompilationUnit& unit) {
  while (!Check(TokenType::kEndOfFile)) {
    auto* statement{ParseStatement()};
    if (statement == nullptr) {
      return false;
    }

    unit.statements.push_back(statement);
  }

  return true;
}

ReplParseResult Parser::ParseReplInput(CompilationUnit& unit) {
  bool has_result{false};

  while (!Check(TokenType::kEndOfFile)) {
    Expression* expression{ParseExpression()};
    if (expression == nullptr) {
      return {};
    }

    const bool terminated{Match(TokenType::kSemicolon)};
    if (!terminated && !Check(TokenType::kEndOfFile)) {
      return {};
    }

    unit.statements.push_back(ast_.CreateExpressionStatement(expression));
    has_result = !terminated;
  }

  return {
      .succeeded = !unit.statements.empty(),
      .has_result = has_result,
  };
}

const Parser::ParseRule& Parser::GetRule(TokenType type) {
  // clang-format off
  static const ParseRule kRules[]{
    // kTrue
    {.prefix = &Parser::ParseBooleanLiteral, .infix = nullptr, .precedence = Precedence::kNone},

    // kFalse
    {.prefix = &Parser::ParseBooleanLiteral, .infix = nullptr, .precedence = Precedence::kNone},

    // kIntegerLiteral
    {.prefix = &Parser::ParseIntegerLiteral, .infix = nullptr, .precedence = Precedence::kNone},

    // kFloatLiteral
    {.prefix = &Parser::ParseFloatLiteral, .infix = nullptr, .precedence = Precedence::kNone},

    // kIdentifier
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kLeftParen
    {.prefix = &Parser::ParseGrouping, .infix = nullptr, .precedence = Precedence::kNone},

    // kRightParen
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kStar
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kFactor},

    // kSlash
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kFactor},

    // kPlus
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kTerm},

    // kMinus
    {.prefix = &Parser::ParseUnary, .infix = &Parser::ParseBinary, .precedence = Precedence::kTerm},

    // kSemicolon
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kEndOfFile
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kInvalid
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},
  };
  // clang-format on

  static_assert(std::size(kRules) == static_cast<usize>(TokenType::kCount));
  return kRules[static_cast<usize>(type)];
}

void Parser::Advance() {
  previous_ = current_;
  current_ = lexer_.NextToken();
}

bool Parser::Check(TokenType type) const { return current_.type == type; }

bool Parser::Match(TokenType type) {
  if (!Check(type)) {
    return false;
  }

  Advance();
  return true;
}

Expression* Parser::ParseExpression() {
  return ParsePrecedence(Precedence::kNone);
}

Expression* Parser::ParsePrecedence(Precedence precedence) {
  Advance();
  const PrefixParseFunction prefix{GetRule(previous_.type).prefix};

  if (prefix == nullptr) {
    return nullptr;
  }

  Expression* left{(this->*prefix)()};

  while (precedence < GetRule(current_.type).precedence) {
    Advance();
    const auto infix{GetRule(previous_.type).infix};

    if (infix == nullptr) {
      return nullptr;
    }

    left = (this->*infix)(left);
  }

  return left;
}

Expression* Parser::ParseBooleanLiteral() {
  FELL_ASSERT(previous_.type == TokenType::kTrue ||
              previous_.type == TokenType::kFalse);

  return ast_.CreateBooleanLiteralExpression(previous_.type ==
                                             TokenType::kTrue);
}

Expression* Parser::ParseIntegerLiteral() {
  StringView lexeme{previous_.lexeme};
  auto explicit_type{Type::kInvalid};

  if (lexeme.ends_with("s8")) {
    explicit_type = Type::kS8;
    lexeme.remove_suffix(2);
  } else if (lexeme.ends_with("s16")) {
    explicit_type = Type::kS16;
    lexeme.remove_suffix(3);
  } else if (lexeme.ends_with("s32")) {
    explicit_type = Type::kS32;
    lexeme.remove_suffix(3);
  } else if (lexeme.ends_with("s64")) {
    explicit_type = Type::kS64;
    lexeme.remove_suffix(3);
  } else if (lexeme.ends_with("u8")) {
    explicit_type = Type::kU8;
    lexeme.remove_suffix(2);
  } else if (lexeme.ends_with("u16")) {
    explicit_type = Type::kU16;
    lexeme.remove_suffix(3);
  } else if (lexeme.ends_with("u32")) {
    explicit_type = Type::kU32;
    lexeme.remove_suffix(3);
  } else if (lexeme.ends_with("u64")) {
    explicit_type = Type::kU64;
    lexeme.remove_suffix(3);
  }

  u64 value{0};
  const auto result{
      std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value)};

  if (result.ec != std::errc{} || result.ptr != lexeme.data() + lexeme.size()) {
    return nullptr;
  }

  return ast_.CreateIntegerLiteralExpression(value, explicit_type);
}

Expression* Parser::ParseFloatLiteral() {
  StringView lexeme{previous_.lexeme};
  auto explicit_type{Type::kInvalid};

  if (lexeme.ends_with("f32")) {
    explicit_type = Type::kF32;
    lexeme.remove_suffix(3);
  } else if (lexeme.ends_with("f64")) {
    explicit_type = Type::kF64;
    lexeme.remove_suffix(3);
  } else if (lexeme.ends_with('f')) {
    explicit_type = Type::kF32;
    lexeme.remove_suffix(1);
  }

  f64 value{0.0};
  const auto result{
      std::from_chars(lexeme.data(), lexeme.data() + lexeme.size(), value)};

  if (result.ec != std::errc{} || result.ptr != lexeme.data() + lexeme.size()) {
    return nullptr;
  }

  return ast_.CreateFloatLiteralExpression(value, explicit_type);
}

Expression* Parser::ParseGrouping() {
  Expression* expression{ParseExpression()};

  if (expression == nullptr || !Match(TokenType::kRightParen)) {
    return nullptr;
  }

  return expression;
}

Expression* Parser::ParseUnary() {
  const TokenType operator_type{previous_.type};
  Expression* const operand{ParsePrecedence(Precedence::kUnary)};

  if (operand == nullptr) {
    return nullptr;
  }

  return ast_.CreateUnaryExpression(GetUnaryOperator(operator_type), operand);
}

Expression* Parser::ParseBinary(Expression* left) {
  const TokenType operator_type{previous_.type};
  const Precedence precedence{GetRule(operator_type).precedence};
  Expression* right{ParsePrecedence(precedence)};

  if (right == nullptr) {
    return nullptr;
  }

  return ast_.CreateBinaryExpression(left, GetBinaryOperator(operator_type),
                                     right);
}

Statement* Parser::ParseStatement() {
  auto* expression{ParseExpression()};

  if (expression == nullptr) {
    return nullptr;
  }

  if (!Match(TokenType::kSemicolon)) {
    return nullptr;
  }

  return ast_.CreateExpressionStatement(expression);
}

}  // namespace fell

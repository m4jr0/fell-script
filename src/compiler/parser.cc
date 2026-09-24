#include "compiler/parser.h"

#include "core/assert.h"
#include "core/core.h"

namespace fell {
namespace {

BinaryOperator GetBinaryOperator(TokenType type) {
  switch (type) {
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

Expression* Parser::ParseReplExpression() {
  Expression* expression{ParseExpression()};

  if (expression == nullptr) {
    return nullptr;
  }

  Match(TokenType::kSemicolon);

  if (!Check(TokenType::kEndOfFile)) {
    return nullptr;
  }

  return expression;
}

const Parser::ParseRule& Parser::GetRule(TokenType type) {
  static const ParseRule kRules[]{
      // kIntegerLiteral
      {
          .prefix = &Parser::ParseIntegerLiteral,
          .infix = nullptr,
          .precedence = Precedence::kNone,
      },

      // kPlus
      {
          .prefix = nullptr,
          .infix = &Parser::ParseBinary,
          .precedence = Precedence::kTerm,
      },

      // kMinus
      {
          .prefix = nullptr,
          .infix = &Parser::ParseBinary,
          .precedence = Precedence::kTerm,
      },

      // kSemicolon
      {
          .prefix = nullptr,
          .infix = nullptr,
          .precedence = Precedence::kNone,
      },

      // kEndOfFile
      {
          .prefix = nullptr,
          .infix = nullptr,
          .precedence = Precedence::kNone,
      },

      // kInvalid
      {
          .prefix = nullptr,
          .infix = nullptr,
          .precedence = Precedence::kNone,
      },
  };

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

Expression* Parser::ParseIntegerLiteral() {
  return ast_.CreateIntegerLiteralExpression(previous_.value);
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

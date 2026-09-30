#include "compiler/parser.h"

#include <charconv>
#include <utility>

#include "core/assert.h"
#include "core/core.h"

namespace fell {
namespace {

SourceSpan MergeSpans(SourceSpan first, SourceSpan last) {
  FELL_ASSERT(first.start.offset <= last.start.offset + last.length);

  return {
      .start = first.start,
      .length = last.start.offset + last.length - first.start.offset,
  };
}

UnaryOperator GetUnaryOperator(TokenType type) {
  switch (type) {
    case TokenType::kMinus:
      return UnaryOperator::kNegate;
    case TokenType::kBang:
      return UnaryOperator::kLogicalNot;
    default:
      FELL_UNREACHABLE();
  }
}

BinaryOperator GetBinaryOperator(TokenType type) {
  switch (type) {
    case TokenType::kAmpAmp:
      return BinaryOperator::kLogicalAnd;
    case TokenType::kPipePipe:
      return BinaryOperator::kLogicalOr;
    case TokenType::kStar:
      return BinaryOperator::kMultiply;
    case TokenType::kSlash:
      return BinaryOperator::kDivide;
    case TokenType::kPlus:
      return BinaryOperator::kAdd;
    case TokenType::kMinus:
      return BinaryOperator::kSubtract;
    case TokenType::kBangEqual:
      return BinaryOperator::kNotEqual;
    case TokenType::kEqualEqual:
      return BinaryOperator::kEqual;
    case TokenType::kLess:
      return BinaryOperator::kLess;
    case TokenType::kLessEqual:
      return BinaryOperator::kLessEqual;
    case TokenType::kGreater:
      return BinaryOperator::kGreater;
    case TokenType::kGreaterEqual:
      return BinaryOperator::kGreaterEqual;
    default:
      FELL_UNREACHABLE();
  }
}

}  // namespace

bool ParseResult::Succeeded() const { return !HasErrors(diagnostics); }

Parser::Parser(Lexer& lexer, Ast& ast) : lexer_(lexer), ast_(ast) { Advance(); }

ParseResult Parser::ParseCompilationUnit(CompilationUnit& unit) {
  while (!Check(TokenType::kEndOfFile)) {
    Statement* const statement{ParseStatement()};

    if (statement != nullptr) {
      unit.statements.push_back(statement);
    } else {
      Synchronize();
    }
  }

  if (unit.statements.empty() && diagnostics_.empty()) {
    ErrorAtCurrent("expected statement");
  }

  return {
      .diagnostics = std::move(diagnostics_),
  };
}

ParseResult Parser::ParseReplInput(CompilationUnit& unit) {
  bool has_result{false};

  while (!Check(TokenType::kEndOfFile)) {
    Expression* const expression{ParseExpression()};

    if (expression == nullptr) {
      Synchronize();
      has_result = false;
      continue;
    }

    const bool terminated{Match(TokenType::kSemicolon)};
    if (!terminated && !Check(TokenType::kEndOfFile)) {
      ErrorAtCurrent("expected ';' or end of input after expression");
      Synchronize();
      has_result = false;
      continue;
    }

    const SourceSpan statement_span{
        terminated ? MergeSpans(expression->span, previous_.span)
                   : expression->span,
    };
    unit.statements.push_back(
        ast_.CreateExpressionStatement(expression, statement_span));
    has_result = !terminated;
  }

  if (unit.statements.empty() && diagnostics_.empty()) {
    ErrorAtCurrent("expected expression");
  }

  return {
      .diagnostics = std::move(diagnostics_),
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

    // kStringLiteral
    {.prefix = &Parser::ParseStringLiteral, .infix = nullptr, .precedence = Precedence::kNone},

    // kIdentifier
    {.prefix = &Parser::ParseVariable, .infix = nullptr, .precedence = Precedence::kNone},

    // kLet
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kMut
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kIf
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kElse
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kWhile
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kBreak
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kContinue
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kFor
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kSwitch
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kCase
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kDefault
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kLeftParen
    {.prefix = &Parser::ParseGrouping, .infix = nullptr, .precedence = Precedence::kNone},

    // kRightParen
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kLeftBrace
    {.prefix = &Parser::ParseBlock, .infix = nullptr, .precedence = Precedence::kNone},

    // kRightBrace
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kStar
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kFactor},

    // kSlash
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kFactor},

    // kPlus
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kTerm},

    // kMinus
    {.prefix = &Parser::ParseUnary, .infix = &Parser::ParseBinary, .precedence = Precedence::kTerm},

    // kAmpAmp
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kLogicalAnd},

    // kPipePipe
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kLogicalOr},

    // kBang
    {.prefix = &Parser::ParseUnary, .infix = nullptr, .precedence = Precedence::kNone},

    // kBangEqual
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kEquality},

    // kEqual
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kEqualEqual
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kEquality},

    // kLess
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kComparison},

    // kLessEqual
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kComparison},

    // kGreater
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kComparison},

    // kGreaterEqual
    {.prefix = nullptr, .infix = &Parser::ParseBinary, .precedence = Precedence::kComparison},

    // kColon
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kSemicolon
    {.prefix = nullptr, .infix = nullptr, .precedence = Precedence::kNone},

    // kQuestion
    {.prefix = nullptr, .infix = &Parser::ParseConditional, .precedence = Precedence::kConditional},

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

void Parser::ErrorAt(const Token& token, StringView message) {
  if (panic_mode_) {
    return;
  }

  panic_mode_ = true;
  diagnostics_.push_back({
      .severity = DiagnosticSeverity::kError,
      .message = String{message},
      .span = token.span,
  });
}

void Parser::ErrorAtCurrent(StringView message) { ErrorAt(current_, message); }

void Parser::ErrorAtPrevious(StringView message) {
  ErrorAt(previous_, message);
}

void Parser::Synchronize() {
  panic_mode_ = false;

  while (!Check(TokenType::kEndOfFile)) {
    if (Check(TokenType::kRightBrace) ||
        previous_.type == TokenType::kSemicolon) {
      return;
    }

    Advance();
  }
}

Expression* Parser::ParseExpression() {
  Expression* expression{ParsePrecedence(Precedence::kNone)};
  if (expression == nullptr || !Match(TokenType::kEqual)) {
    return expression;
  }

  if (expression->kind != ExpressionKind::kVariable) {
    ErrorAtPrevious("invalid assignment target");
    return nullptr;
  }

  Expression* const value{ParseExpression()};
  if (value == nullptr) {
    return nullptr;
  }

  return ast_.CreateAssignmentExpression(
      expression->variable.name, value,
      MergeSpans(expression->span, value->span));
}

Expression* Parser::ParsePrecedence(Precedence precedence) {
  Advance();
  const PrefixParseFunction prefix{GetRule(previous_.type).prefix};

  if (prefix == nullptr) {
    ErrorAtPrevious("expected expression");
    return nullptr;
  }

  Expression* left{(this->*prefix)()};
  if (left == nullptr) {
    return nullptr;
  }

  while (precedence < GetRule(current_.type).precedence) {
    Advance();
    const InfixParseFunction infix{GetRule(previous_.type).infix};

    if (infix == nullptr) {
      ErrorAtPrevious("expected operator");
      return nullptr;
    }

    left = (this->*infix)(left);
    if (left == nullptr) {
      return nullptr;
    }
  }

  return left;
}

Expression* Parser::ParseBooleanLiteral() {
  FELL_ASSERT(previous_.type == TokenType::kTrue ||
              previous_.type == TokenType::kFalse);

  return ast_.CreateBooleanLiteralExpression(previous_.type == TokenType::kTrue,
                                             previous_.span);
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
    ErrorAtPrevious("invalid integer literal");
    return nullptr;
  }

  return ast_.CreateIntegerLiteralExpression(value, explicit_type,
                                             previous_.span);
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
    ErrorAtPrevious("invalid floating-point literal");
    return nullptr;
  }

  return ast_.CreateFloatLiteralExpression(value, explicit_type,
                                           previous_.span);
}

Expression* Parser::ParseStringLiteral() {
  FELL_ASSERT(previous_.type == TokenType::kStringLiteral);
  FELL_ASSERT(previous_.lexeme.size() >= 2);

  return ast_.CreateStringLiteralExpression(
      previous_.lexeme.substr(1, previous_.lexeme.size() - 2), previous_.span);
}

Expression* Parser::ParseVariable() {
  FELL_ASSERT(previous_.type == TokenType::kIdentifier);
  return ast_.CreateVariableExpression(previous_.lexeme, previous_.span);
}

Expression* Parser::ParseGrouping() {
  const SourceSpan opening_span{previous_.span};
  Expression* const expression{ParseExpression()};

  if (expression == nullptr) {
    return nullptr;
  }

  if (!Match(TokenType::kRightParen)) {
    ErrorAtCurrent("expected ')' after expression");
    return nullptr;
  }

  expression->span = MergeSpans(opening_span, previous_.span);
  return expression;
}

Expression* Parser::ParseBlock() {
  const SourceSpan opening_span{previous_.span};
  CompilationUnit* const body{ast_.CreateCompilationUnit()};
  Expression* trailing_expression{nullptr};

  while (!Check(TokenType::kRightBrace) && !Check(TokenType::kEndOfFile)) {
    if (Match(TokenType::kLet)) {
      Statement* const declaration{ParseVariableDeclaration()};
      if (declaration == nullptr) return nullptr;
      body->statements.push_back(declaration);
      continue;
    }
    if (Check(TokenType::kIf) || Check(TokenType::kWhile) ||
        Check(TokenType::kBreak) || Check(TokenType::kContinue) ||
        Check(TokenType::kSwitch)) {
      Statement* const statement{ParseStatement()};
      if (statement == nullptr) return nullptr;
      body->statements.push_back(statement);
      continue;
    }

    Expression* const expression{ParseExpression()};
    if (expression == nullptr) {
      return nullptr;
    }

    if (Match(TokenType::kSemicolon)) {
      body->statements.push_back(ast_.CreateExpressionStatement(
          expression, MergeSpans(expression->span, previous_.span)));
      continue;
    }

    if (!Check(TokenType::kRightBrace)) {
      if (expression->kind == ExpressionKind::kBlock) {
        body->statements.push_back(
            ast_.CreateExpressionStatement(expression, expression->span));
        continue;
      }
      ErrorAtCurrent("expected ';' or '}' after expression");
      return nullptr;
    }

    trailing_expression = expression;
    break;
  }

  if (!Match(TokenType::kRightBrace)) {
    ErrorAtCurrent("expected '}' after block");
    return nullptr;
  }

  return ast_.CreateBlockExpression(body, trailing_expression,
                                    MergeSpans(opening_span, previous_.span));
}

Expression* Parser::ParseUnary() {
  const TokenType operator_type{previous_.type};
  const SourceSpan operator_span{previous_.span};
  Expression* const operand{ParsePrecedence(Precedence::kUnary)};

  if (operand == nullptr) {
    return nullptr;
  }

  return ast_.CreateUnaryExpression(GetUnaryOperator(operator_type), operand,
                                    MergeSpans(operator_span, operand->span));
}

Expression* Parser::ParseBinary(Expression* left) {
  const TokenType operator_type{previous_.type};
  const Precedence precedence{GetRule(operator_type).precedence};
  Expression* const right{ParsePrecedence(precedence)};

  if (right == nullptr) {
    return nullptr;
  }

  return ast_.CreateBinaryExpression(left, GetBinaryOperator(operator_type),
                                     right,
                                     MergeSpans(left->span, right->span));
}

Expression* Parser::ParseConditional(Expression* condition) {
  Expression* const then_expression{ParseExpression()};
  if (then_expression == nullptr) {
    return nullptr;
  }
  if (!Match(TokenType::kColon)) {
    ErrorAtCurrent("expected ':' in conditional expression");
    return nullptr;
  }
  Expression* const else_expression{ParseExpression()};
  if (else_expression == nullptr) {
    return nullptr;
  }
  return ast_.CreateConditionalExpression(
      condition, then_expression, else_expression,
      MergeSpans(condition->span, else_expression->span));
}

Statement* Parser::ParseStatement() {
  if (Match(TokenType::kLet)) {
    return ParseVariableDeclaration();
  }
  if (Match(TokenType::kIf)) {
    return ParseIfStatement();
  }
  if (Match(TokenType::kWhile)) {
    return ParseWhileStatement();
  }
  if (Match(TokenType::kFor)) {
    return ParseForStatement();
  }
  if (Match(TokenType::kBreak)) {
    return ParseBreakStatement();
  }
  if (Match(TokenType::kContinue)) {
    return ParseContinueStatement();
  }
  if (Match(TokenType::kSwitch)) {
    return ParseSwitchStatement();
  }

  Expression* const expression{ParseExpression()};

  if (expression == nullptr) {
    return nullptr;
  }

  if (expression->kind == ExpressionKind::kBlock &&
      !Check(TokenType::kSemicolon)) {
    return ast_.CreateExpressionStatement(expression, expression->span);
  }

  if (!Match(TokenType::kSemicolon)) {
    ErrorAtCurrent("expected ';' after expression");
    return nullptr;
  }

  return ast_.CreateExpressionStatement(
      expression, MergeSpans(expression->span, previous_.span));
}

Statement* Parser::ParseIfStatement() {
  const SourceSpan if_span{previous_.span};
  Expression* const condition{ParseExpression()};
  if (condition == nullptr) return nullptr;
  if (!Match(TokenType::kLeftBrace)) {
    ErrorAtCurrent("expected '{' after if condition");
    return nullptr;
  }
  Expression* const then_block{ParseBlock()};
  if (then_block == nullptr) return nullptr;

  Expression* else_block{nullptr};
  if (Match(TokenType::kElse)) {
    if (!Match(TokenType::kLeftBrace)) {
      ErrorAtCurrent("expected '{' after 'else'");
      return nullptr;
    }
    else_block = ParseBlock();
    if (else_block == nullptr) return nullptr;
  }
  return ast_.CreateIfStatement(
      condition, then_block, else_block,
      MergeSpans(if_span,
                 (else_block != nullptr ? else_block : then_block)->span));
}

Statement* Parser::ParseWhileStatement() {
  const SourceSpan while_span{previous_.span};
  Expression* const condition{ParseExpression()};
  if (condition == nullptr) return nullptr;
  if (!Match(TokenType::kLeftBrace)) {
    ErrorAtCurrent("expected '{' after while condition");
    return nullptr;
  }
  Expression* const body{ParseBlock()};
  if (body == nullptr) return nullptr;
  return ast_.CreateWhileStatement(condition, body,
                                   MergeSpans(while_span, body->span));
}

Statement* Parser::ParseForStatement() {
  const SourceSpan for_span{previous_.span};
  if (!Match(TokenType::kLeftParen)) {
    ErrorAtCurrent("expected '(' after 'for'");
    return nullptr;
  }

  Statement* initializer{nullptr};
  if (Match(TokenType::kSemicolon)) {
  } else if (Match(TokenType::kLet)) {
    initializer = ParseVariableDeclaration();
    if (initializer == nullptr) return nullptr;
  } else {
    Expression* const expression{ParseExpression()};
    if (expression == nullptr) return nullptr;
    if (!Match(TokenType::kSemicolon)) {
      ErrorAtCurrent("expected ';' after for initializer");
      return nullptr;
    }
    initializer = ast_.CreateExpressionStatement(
        expression, MergeSpans(expression->span, previous_.span));
  }

  Expression* condition{nullptr};
  if (!Check(TokenType::kSemicolon)) {
    condition = ParseExpression();
    if (condition == nullptr) return nullptr;
  }
  if (!Match(TokenType::kSemicolon)) {
    ErrorAtCurrent("expected ';' after for condition");
    return nullptr;
  }

  Expression* increment{nullptr};
  if (!Check(TokenType::kRightParen)) {
    increment = ParseExpression();
    if (increment == nullptr) return nullptr;
  }
  if (!Match(TokenType::kRightParen)) {
    ErrorAtCurrent("expected ')' after for clauses");
    return nullptr;
  }
  if (!Match(TokenType::kLeftBrace)) {
    ErrorAtCurrent("expected '{' after for clauses");
    return nullptr;
  }
  Expression* const body{ParseBlock()};
  if (body == nullptr) return nullptr;
  return ast_.CreateForStatement(initializer, condition, increment, body,
                                 MergeSpans(for_span, body->span));
}

Statement* Parser::ParseBreakStatement() {
  const SourceSpan keyword_span{previous_.span};
  if (!Match(TokenType::kSemicolon)) {
    ErrorAtCurrent("expected ';' after 'break'");
    return nullptr;
  }
  return ast_.CreateBreakStatement(MergeSpans(keyword_span, previous_.span));
}

Statement* Parser::ParseContinueStatement() {
  const SourceSpan keyword_span{previous_.span};
  if (!Match(TokenType::kSemicolon)) {
    ErrorAtCurrent("expected ';' after 'continue'");
    return nullptr;
  }
  return ast_.CreateContinueStatement(MergeSpans(keyword_span, previous_.span));
}

Statement* Parser::ParseSwitchStatement() {
  const SourceSpan switch_span{previous_.span};
  Expression* const value{ParseExpression()};
  if (value == nullptr) return nullptr;
  if (!Match(TokenType::kLeftBrace)) {
    ErrorAtCurrent("expected '{' after switch value");
    return nullptr;
  }

  Vector<SwitchCase> cases;
  Expression* default_body{nullptr};
  while (!Check(TokenType::kRightBrace) && !Check(TokenType::kEndOfFile)) {
    if (Match(TokenType::kCase)) {
      Expression* const case_value{ParseExpression()};
      if (case_value == nullptr) return nullptr;
      if (!Match(TokenType::kLeftBrace)) {
        ErrorAtCurrent("expected '{' after case value");
        return nullptr;
      }
      Expression* const body{ParseBlock()};
      if (body == nullptr) return nullptr;
      cases.push_back({.value = case_value, .body = body});
      continue;
    }
    if (Match(TokenType::kDefault)) {
      if (default_body != nullptr) {
        ErrorAtPrevious("duplicate default case");
        return nullptr;
      }
      if (!Match(TokenType::kLeftBrace)) {
        ErrorAtCurrent("expected '{' after 'default'");
        return nullptr;
      }
      default_body = ParseBlock();
      if (default_body == nullptr) return nullptr;
      continue;
    }
    ErrorAtCurrent("expected 'case', 'default', or '}' in switch");
    return nullptr;
  }
  if (!Match(TokenType::kRightBrace)) {
    ErrorAtCurrent("expected '}' after switch");
    return nullptr;
  }
  return ast_.CreateSwitchStatement(value, std::move(cases), default_body,
                                    MergeSpans(switch_span, previous_.span));
}

Statement* Parser::ParseVariableDeclaration() {
  const SourceSpan let_span{previous_.span};
  const bool is_mutable{Match(TokenType::kMut)};

  if (!Match(TokenType::kIdentifier)) {
    ErrorAtCurrent("expected variable name after 'let'");
    return nullptr;
  }
  const Token name{previous_};

  Type explicit_type{Type::kInvalid};
  if (Match(TokenType::kColon)) {
    explicit_type = ParseType();
    if (explicit_type == Type::kInvalid) {
      return nullptr;
    }
  }

  if (!Match(TokenType::kEqual)) {
    ErrorAtCurrent("expected '=' after variable name");
    return nullptr;
  }

  Expression* const initializer{ParseExpression()};
  if (initializer == nullptr) {
    return nullptr;
  }

  if (!Match(TokenType::kSemicolon)) {
    ErrorAtCurrent("expected ';' after variable declaration");
    return nullptr;
  }

  return ast_.CreateVariableDeclarationStatement(
      name.lexeme, explicit_type, initializer, is_mutable,
      MergeSpans(let_span, previous_.span));
}

Type Parser::ParseType() {
  if (!Match(TokenType::kIdentifier)) {
    ErrorAtCurrent("expected type name after ':'");
    return Type::kInvalid;
  }

  const StringView name{previous_.lexeme};
  if (name == "bool") return Type::kBool;
  if (name == "s8") return Type::kS8;
  if (name == "s16") return Type::kS16;
  if (name == "s32") return Type::kS32;
  if (name == "s64") return Type::kS64;
  if (name == "u8") return Type::kU8;
  if (name == "u16") return Type::kU16;
  if (name == "u32") return Type::kU32;
  if (name == "u64") return Type::kU64;
  if (name == "f32") return Type::kF32;
  if (name == "f64") return Type::kF64;
  if (name == "string") return Type::kString;

  ErrorAtPrevious("unknown type name");
  return Type::kInvalid;
}

}  // namespace fell

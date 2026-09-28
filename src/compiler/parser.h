#pragma once

#include "compiler/ast.h"
#include "compiler/diagnostic.h"
#include "compiler/lexer.h"
#include "core/vector.h"

namespace fell {

struct ParseResult {
  Vector<Diagnostic> diagnostics;
  bool has_result{false};

  bool Succeeded() const;
};

class Parser {
 public:
  Parser(Lexer& lexer, Ast& ast);

  ParseResult ParseCompilationUnit(CompilationUnit& unit);
  ParseResult ParseReplInput(CompilationUnit& unit);

 private:
  enum class Precedence {
    kNone,
    kEquality,
    kComparison,
    kTerm,
    kFactor,
    kUnary,
  };

  using PrefixParseFunction = Expression* (Parser::*)();
  using InfixParseFunction = Expression* (Parser::*)(Expression*);

  struct ParseRule {
    PrefixParseFunction prefix;
    InfixParseFunction infix;
    Precedence precedence;
  };

  static const ParseRule& GetRule(TokenType type);

  void Advance();
  bool Match(TokenType type);
  bool Check(TokenType type) const;

  void ErrorAt(const Token& token, StringView message);
  void ErrorAtCurrent(StringView message);
  void ErrorAtPrevious(StringView message);
  void Synchronize();

  Expression* ParseExpression();
  Expression* ParsePrecedence(Precedence precedence);

  Expression* ParseBooleanLiteral();
  Expression* ParseIntegerLiteral();
  Expression* ParseFloatLiteral();
  Expression* ParseGrouping();
  Expression* ParseUnary();
  Expression* ParseBinary(Expression* left);

  Statement* ParseStatement();

  Token current_;
  Token previous_;

  Lexer& lexer_;
  Ast& ast_;

  Vector<Diagnostic> diagnostics_;
  bool panic_mode_{false};
};

}  // namespace fell

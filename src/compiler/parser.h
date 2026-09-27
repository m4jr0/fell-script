#pragma once

#include "compiler/ast.h"
#include "compiler/lexer.h"

namespace fell {

struct ReplParseResult {
  bool succeeded{false};
  bool has_result{false};
};

class Parser {
 public:
  Parser(Lexer& lexer, Ast& ast);

  bool ParseCompilationUnit(CompilationUnit& unit);
  ReplParseResult ParseReplInput(CompilationUnit& unit);

 private:
  enum class Precedence {
    kNone,
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
};

}  // namespace fell

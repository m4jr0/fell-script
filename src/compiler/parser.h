#pragma once

#include "compiler/ast.h"
#include "compiler/lexer.h"

namespace fell {

class Parser {
 public:
  Parser(Lexer& lexer, Ast& ast);

  bool ParseCompilationUnit(CompilationUnit& unit);
  Expression* ParseReplExpression();

 private:
  enum class Precedence {
    kNone,
    kTerm,
  };

  using PrefixParseFunction = Expression* (Parser::*)();
  using InfixParseFunction = Expression* (Parser::*)(Expression * left);

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

  Expression* ParseIntegerLiteral();
  Expression* ParseFloatLiteral();
  Expression* ParseBinary(Expression* left);

  Statement* ParseStatement();

  Token current_;
  Token previous_;

  Lexer& lexer_;
  Ast& ast_;
};

}  // namespace fell

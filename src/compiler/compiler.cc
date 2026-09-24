#include "compiler/compiler.h"

#include "compiler/bytecode_compiler.h"
#include "compiler/ir_builder.h"
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/semantic_analyzer.h"

namespace fell {

bool CompileResult::Succeeded() const {
  for (const Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == DiagnosticSeverity::kError) {
      return false;
    }
  }

  return true;
}

CompileResult Compiler::Compile(StringView source) {
  Lexer lexer{source};

  Ast ast{};
  Parser parser{lexer, ast};

  CompilationUnit unit{};

  if (!parser.ParseCompilationUnit(unit) || unit.statements.empty()) {
    return {
        .program = {},
        .diagnostics =
            {
                {
                    .severity = DiagnosticSeverity::kError,
                    .message = "expected a valid compilation unit",
                },
            },
    };
  }

  return CompileUnit(unit);
}

CompileResult Compiler::CompileExpression(StringView source) {
  Lexer lexer{source};

  Ast ast{};
  Parser parser{lexer, ast};

  Expression* expression{parser.ParseReplExpression()};

  if (expression == nullptr) {
    return {
        .program = {},
        .diagnostics =
            {
                {
                    .severity = DiagnosticSeverity::kError,
                    .message = "expected a valid expression",
                },
            },
    };
  }

  CompilationUnit unit{};
  unit.statements.push_back(ast.CreateExpressionStatement(expression));

  return CompileUnit(unit);
}

CompileResult Compiler::CompileUnit(const CompilationUnit& unit) {
  SemanticAnalyzer semantic_analyzer{};
  const auto semantics{semantic_analyzer.Analyze(unit)};

  IrBuilder ir_builder{};
  const auto ir{ir_builder.Build(unit, semantics)};

  BytecodeCompiler bytecode_compiler{};

  return {
      .program = bytecode_compiler.Compile(ir),
      .diagnostics = {},
  };
}

}  // namespace fell

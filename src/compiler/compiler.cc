#include "compiler/compiler.h"

#include <utility>

#include "compiler/bytecode_compiler.h"
#include "compiler/ir_builder.h"
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/semantic_analyzer.h"

namespace fell {

bool CompileResult::Succeeded() const { return !HasErrors(diagnostics); }

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
  auto semantic_result{semantic_analyzer.Analyze(unit)};

  if (HasErrors(semantic_result.diagnostics)) {
    return {
        .program = {},
        .diagnostics = std::move(semantic_result.diagnostics),
    };
  }

  IrBuilder ir_builder{};
  const auto ir{ir_builder.Build(unit, semantic_result.model)};

  BytecodeCompiler bytecode_compiler{};

  return {
      .program = bytecode_compiler.Compile(ir),
      .diagnostics = std::move(semantic_result.diagnostics),
  };
}

}  // namespace fell
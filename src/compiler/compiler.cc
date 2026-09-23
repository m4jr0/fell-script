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
  Lexer lexer(source);
  Parser parser(lexer);

  IntegerExpression expression;

  if (!parser.ParseExpression(expression)) {
    return {
        .program = {},
        .diagnostics =
            {
                {
                    .severity = DiagnosticSeverity::kError,
                    .message = "expected a single integer literal",
                },
            },
    };
  }

  SemanticAnalyzer semantic_analyzer;
  const TypedIntegerExpression typed_expression =
      semantic_analyzer.Analyze(expression);

  IrBuilder ir_builder;
  const IrProgram ir = ir_builder.Build(typed_expression);

  BytecodeCompiler bytecode_compiler;

  return {
      .program = bytecode_compiler.Compile(ir),
      .diagnostics = {},
  };
}

}  // namespace fell
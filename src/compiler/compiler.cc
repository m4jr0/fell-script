#include "compiler/compiler.h"

#include <utility>

#include "bytecode/dump.h"
#include "compiler/ast.h"
#include "compiler/bytecode_compiler.h"
#include "compiler/dump.h"
#include "compiler/ir_builder.h"
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/semantic_analyzer.h"

namespace fell {
namespace {

struct ParsedProgram {
  Ast ast;
  CompilationUnit unit;
};

bool ParseProgram(StringView source, ParsedProgram& program) {
  Lexer lexer{source};
  Parser parser{lexer, program.ast};
  return parser.ParseCompilationUnit(program.unit) &&
         !program.unit.statements.empty();
}

}  // namespace

bool CompileResult::Succeeded() const { return !HasErrors(diagnostics); }

CompileResult Compiler::Compile(StringView source,
                                const CompileOptions& options) {
  CompileResult result{};

  if (options.dumps.Has(CompileDump::kTokens)) {
    result.token_dump = DumpTokens(source);
  }

  ParsedProgram parsed{};
  if (!ParseProgram(source, parsed)) {
    result.diagnostics.push_back({
        .severity = DiagnosticSeverity::kError,
        .message = "expected a valid compilation unit",
    });
    return result;
  }

  if (options.dumps.Has(CompileDump::kAst)) {
    result.ast_dump = DumpAst(parsed.unit);
  }

  CompileResult compiled{CompileUnit(parsed.unit, options)};
  compiled.token_dump = std::move(result.token_dump);
  compiled.ast_dump = std::move(result.ast_dump);
  return compiled;
}

CompileResult Compiler::CompileReplInput(StringView source,
                                         const CompileOptions& options) {
  CompileResult result{};

  if (options.dumps.Has(CompileDump::kTokens)) {
    result.token_dump = DumpTokens(source);
  }

  ParsedProgram parsed{};
  Lexer lexer{source};
  Parser parser{lexer, parsed.ast};
  const ReplParseResult parse_result{parser.ParseReplInput(parsed.unit)};

  if (!parse_result.succeeded) {
    result.diagnostics.push_back({
        .severity = DiagnosticSeverity::kError,
        .message = "expected valid REPL input",
    });
    return result;
  }

  if (options.dumps.Has(CompileDump::kAst)) {
    result.ast_dump = DumpAst(parsed.unit);
  }

  CompileResult compiled{
      CompileUnit(parsed.unit, options, parse_result.has_result)};
  compiled.token_dump = std::move(result.token_dump);
  compiled.ast_dump = std::move(result.ast_dump);
  return compiled;
}

CompileResult Compiler::CompileUnit(const CompilationUnit& unit,
                                    const CompileOptions& options,
                                    bool return_last_expression) {
  CompileResult result{};

  SemanticAnalyzer semantic_analyzer{};
  auto semantic_result{semantic_analyzer.Analyze(unit)};
  result.diagnostics = std::move(semantic_result.diagnostics);

  if (HasErrors(result.diagnostics)) {
    return result;
  }

  IrBuilder ir_builder{};
  const IrProgram ir{
      ir_builder.Build(unit, semantic_result.model, return_last_expression)};

  if (options.dumps.Has(CompileDump::kIr)) {
    result.ir_dump = DumpIr(ir);
  }

  BytecodeCompiler bytecode_compiler{};
  result.program = bytecode_compiler.Compile(ir);

  if (options.dumps.Has(CompileDump::kBytecode)) {
    result.bytecode_dump = DumpBytecode(result.program);
  }

  return result;
}

}  // namespace fell

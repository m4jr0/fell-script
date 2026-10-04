#include "compiler/compiler.h"

#include <utility>

#include "bytecode/dump.h"
#include "compiler/backend/bytecode_compiler.h"
#include "compiler/frontend/ast.h"
#include "compiler/frontend/lexer.h"
#include "compiler/frontend/parser.h"
#include "compiler/frontend/semantic_analyzer.h"
#include "compiler/ir/dump.h"
#include "compiler/ir/ir_builder.h"
#include "compiler/ir/ir_verifier.h"
#include "compiler/ir/transform/phi_placement.h"

namespace fell {
namespace {

struct ParsedProgram {
  Ast ast;
  CompilationUnit unit;
};

ParseResult ParseProgram(StringView source, ParsedProgram& program) {
  Lexer lexer{source};
  Parser parser{lexer, program.ast};
  return parser.ParseCompilationUnit(program.unit);
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
  ParseResult parse_result{ParseProgram(source, parsed)};
  const bool parse_succeeded{parse_result.Succeeded()};
  result.diagnostics = std::move(parse_result.diagnostics);

  if (!parse_succeeded) {
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
  ParseResult parse_result{parser.ParseReplInput(parsed.unit)};
  const bool parse_succeeded{parse_result.Succeeded()};
  result.diagnostics = std::move(parse_result.diagnostics);

  if (!parse_succeeded) {
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
  IrProgram ir{
      ir_builder.Build(unit, semantic_result.model, return_last_expression)};

  VerifyIr(ir);

  PlacePhiNodes(ir.main);

  for (IrFunction& function : ir.functions) {
    PlacePhiNodes(function.procedure, function.parameter_local_slots);
  }

  VerifyIr(ir);

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

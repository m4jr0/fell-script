#include "command_line.h"

#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <utility>

#include "bytecode/dump.h"
#include "bytecode/serialization.h"
#include "compiler/bytecode_compiler.h"
#include "compiler/compiler.h"
#include "compiler/diagnostic.h"
#include "compiler/dump.h"
#include "compiler/ir_builder.h"
#include "compiler/lexer.h"
#include "compiler/parser.h"
#include "compiler/semantic_analyzer.h"
#include "core/span.h"
#include "core/string.h"
#include "core/vector.h"
#include "runtime/value.h"
#include "runtime/vm.h"
#include "version.h"

namespace fell::tool {
namespace {

void PrintHelp() {
  std::cout << "Fell " << kVersion << '\n'
            << '\n'
            << "Usage:\n"
            << "  fell\n"
            << "  fell repl [dump options]\n"
            << "  fell run <file> [dump options]\n"
            << "  fell compile <file> [-o <file>] [dump options]\n"
            << "  fell dump <tokens|ast|ir|bytecode> <file>\n"
            << '\n'
            << "Options:\n"
            << "  -h, --help         Show this help\n"
            << "  --version          Show the Fell version\n"
            << "  --dump-tokens      Dump lexer tokens\n"
            << "  --dump-ast         Dump the parsed AST\n"
            << "  --dump-ir          Dump the typed IR\n"
            << "  --dump-bytecode    Dump bytecode\n"
            << "  --dump-all         Dump all compiler stages\n";
}

bool ReadTextFile(const char* path, String& output) {
  std::ifstream file{path};

  if (!file) {
    return false;
  }

  std::ostringstream stream{};
  stream << file.rdbuf();
  output = stream.str();
  return true;
}

bool ReadBinaryFile(const char* path, Vector<u8>& output) {
  std::ifstream file{path, std::ios::binary};

  if (!file) {
    return false;
  }

  file.seekg(0, std::ios::end);
  const auto size{file.tellg()};

  if (size < 0) {
    return false;
  }

  file.seekg(0, std::ios::beg);
  output.resize(static_cast<usize>(size));

  if (!output.empty()) {
    file.read(reinterpret_cast<char*>(output.data()),
              static_cast<std::streamsize>(size));
  }

  return file.good() || file.eof();
}

bool WriteBinaryFile(const char* path, Span<const u8> data) {
  std::ofstream file{path, std::ios::binary};

  if (!file) {
    return false;
  }

  if (!data.empty()) {
    file.write(reinterpret_cast<const char*>(data.data()),
               static_cast<std::streamsize>(data.size()));
  }

  return file.good();
}

void PrintDiagnostics(Span<const Diagnostic> diagnostics) {
  for (const Diagnostic& diagnostic : diagnostics) {
    std::cerr << ToString(diagnostic.severity) << ": " << diagnostic.message
              << '\n';
  }
}

void PrintDump(StringView name, StringView dump) {
  if (dump.empty()) {
    return;
  }

  std::cout << "== " << name << " ==\n" << dump << '\n';
}

void PrintCompileDumps(const CompileResult& result) {
  PrintDump("Tokens", result.token_dump);
  PrintDump("AST", result.ast_dump);
  PrintDump("IR", result.ir_dump);
  PrintDump("Bytecode", result.bytecode_dump);
}

bool ParseDumpOption(StringView argument, CompileOptions& options) {
  if (argument == "--dump-tokens") {
    options.dumps.Set(CompileDump::kTokens);
    return true;
  }

  if (argument == "--dump-ast") {
    options.dumps.Set(CompileDump::kAst);
    return true;
  }

  if (argument == "--dump-ir") {
    options.dumps.Set(CompileDump::kIr);
    return true;
  }

  if (argument == "--dump-bytecode") {
    options.dumps.Set(CompileDump::kBytecode);
    return true;
  }

  if (argument == "--dump-all") {
    options.dumps.Set(CompileDump::kTokens);
    options.dumps.Set(CompileDump::kAst);
    options.dumps.Set(CompileDump::kIr);
    options.dumps.Set(CompileDump::kBytecode);
    return true;
  }

  return false;
}

bool ParseDumpOptions(int argc, char* argv[], int first_argument,
                      CompileOptions& options) {
  for (int index{first_argument}; index < argc; ++index) {
    if (!ParseDumpOption(argv[index], options)) {
      return false;
    }
  }

  return true;
}

std::optional<Value> ExecuteModule(const BytecodeModule& module) {
  Vm vm{};
  return vm.Execute(module);
}

int RunSource(StringView source, const CompileOptions& options = {}) {
  Compiler compiler{};
  CompileResult result{compiler.Compile(source, options)};

  PrintCompileDumps(result);
  PrintDiagnostics(result.diagnostics);

  if (!result.Succeeded()) {
    return 1;
  }

  ExecuteModule(result.program);
  return 0;
}

int RunSourceFile(const char* path, const CompileOptions& options) {
  String source{};

  if (!ReadTextFile(path, source)) {
    std::cerr << "error: failed to open source file\n";
    return 1;
  }

  return RunSource(source, options);
}

int RunRepl(const CompileOptions& options = {}) {
  String line{};

  while (true) {
    std::cout << "fell> " << std::flush;

    if (!std::getline(std::cin, line)) {
      std::cout << '\n';
      break;
    }

    if (line.empty()) {
      continue;
    }

    Compiler compiler{};
    CompileResult result{compiler.CompileReplInput(line, options)};

    PrintCompileDumps(result);
    PrintDiagnostics(result.diagnostics);

    if (!result.Succeeded()) {
      continue;
    }

    const std::optional<Value> value{ExecuteModule(result.program)};
    if (value.has_value()) {
      std::cout << ToString(*value) << '\n';
    }
  }

  return 0;
}

int CompileFile(const char* input_path, const char* output_path,
                const CompileOptions& options) {
  String source{};

  if (!ReadTextFile(input_path, source)) {
    std::cerr << "error: failed to open source file\n";
    return 1;
  }

  Compiler compiler{};
  CompileResult result{compiler.Compile(source, options)};

  PrintCompileDumps(result);
  PrintDiagnostics(result.diagnostics);

  if (!result.Succeeded()) {
    return 1;
  }

  const Vector<u8> bytecode{SerializeBytecode(result.program)};

  if (!WriteBinaryFile(output_path, bytecode)) {
    std::cerr << "error: failed to write bytecode file\n";
    return 1;
  }

  return 0;
}

int RunBytecodeFile(const char* path, const CompileOptions& options) {
  Vector<u8> data{};

  if (!ReadBinaryFile(path, data)) {
    std::cerr << "error: failed to open bytecode file\n";
    return 1;
  }

  BytecodeReadResult result{DeserializeBytecode(data)};

  if (!result.Succeeded()) {
    std::cerr << "error: " << result.error << '\n';
    return 1;
  }

  if (options.dumps.Has(CompileDump::kBytecode)) {
    std::cout << "== Bytecode ==\n" << DumpBytecode(result.module);
  }

  ExecuteModule(result.module);
  return 0;
}

int RunFile(const char* path, const CompileOptions& options) {
  const StringView file_path{path};

  if (file_path.ends_with(".fell")) {
    return RunSourceFile(path, options);
  }

  if (file_path.ends_with(".fellc")) {
    if (options.dumps.HasSourceDump()) {
      std::cerr << "error: source dump options require a Fell source file\n";
      return 1;
    }

    return RunBytecodeFile(path, options);
  }

  std::cerr << "error: unsupported file type\n";
  return 1;
}

bool ParseCompilationUnit(StringView source, Ast& ast, CompilationUnit& unit) {
  Lexer lexer{source};
  Parser parser{lexer, ast};
  return parser.ParseCompilationUnit(unit) && !unit.statements.empty();
}

bool BuildIr(StringView source, IrProgram& ir,
             Vector<Diagnostic>& diagnostics) {
  Ast ast{};
  CompilationUnit unit{};

  if (!ParseCompilationUnit(source, ast, unit)) {
    diagnostics.push_back({
        .severity = DiagnosticSeverity::kError,
        .message = "expected a valid compilation unit",
    });
    return false;
  }

  SemanticAnalyzer analyzer{};
  auto semantic_result{analyzer.Analyze(unit)};
  diagnostics = std::move(semantic_result.diagnostics);

  if (HasErrors(diagnostics)) {
    return false;
  }

  IrBuilder builder{};
  ir = builder.Build(unit, semantic_result.model);
  return true;
}

int DumpFile(StringView kind, const char* path) {
  String source{};

  if (!ReadTextFile(path, source)) {
    std::cerr << "error: failed to open source file\n";
    return 1;
  }

  if (kind == "tokens") {
    std::cout << DumpTokens(source);
    return 0;
  }

  if (kind == "ast") {
    Ast ast{};
    CompilationUnit unit{};

    if (!ParseCompilationUnit(source, ast, unit)) {
      std::cerr << "error: expected a valid compilation unit\n";
      return 1;
    }

    std::cout << DumpAst(unit);
    return 0;
  }

  IrProgram ir{};
  Vector<Diagnostic> diagnostics{};

  if (!BuildIr(source, ir, diagnostics)) {
    PrintDiagnostics(diagnostics);
    return 1;
  }

  if (kind == "ir") {
    std::cout << DumpIr(ir);
    return 0;
  }

  if (kind == "bytecode") {
    BytecodeCompiler compiler{};
    std::cout << DumpBytecode(compiler.Compile(ir));
    return 0;
  }

  std::cerr << "error: unknown dump kind '" << kind << "'\n";
  return 1;
}

String DefaultOutputPath(StringView input) {
  const usize slash{input.find_last_of("/\\")};
  const usize dot{input.find_last_of('.')};

  if (dot != StringView::npos && (slash == StringView::npos || dot > slash)) {
    return String{input.substr(0, dot)} + ".fellc";
  }

  return String{input} + ".fellc";
}

}  // namespace

int RunCommandLine(int argc, char* argv[]) {
  if (argc == 1) {
    return RunRepl();
  }

  const StringView command{argv[1]};

  if (command == "-h" || command == "--help") {
    PrintHelp();
    return 0;
  }

  if (command == "--version") {
    std::cout << "Fell " << kVersion << '\n';
    return 0;
  }

  if (command == "repl") {
    CompileOptions options{};

    if (!ParseDumpOptions(argc, argv, 2, options)) {
      std::cerr << "error: invalid repl option\n";
      return 1;
    }

    return RunRepl(options);
  }

  if (command == "run") {
    if (argc < 3) {
      std::cerr << "usage: fell run <file> [dump options]\n";
      return 1;
    }

    CompileOptions options{};

    if (!ParseDumpOptions(argc, argv, 3, options)) {
      std::cerr << "error: invalid run option\n";
      return 1;
    }

    return RunFile(argv[2], options);
  }

  if (command == "compile") {
    if (argc < 3) {
      std::cerr << "usage: fell compile <file> [-o <file>] [dump options]\n";
      return 1;
    }

    String output{DefaultOutputPath(argv[2])};
    CompileOptions options{};

    for (int index{3}; index < argc; ++index) {
      const StringView argument{argv[index]};

      if (argument == "-o") {
        if (index + 1 >= argc) {
          std::cerr << "error: expected output file after -o\n";
          return 1;
        }

        output = argv[++index];
        continue;
      }

      if (!ParseDumpOption(argument, options)) {
        std::cerr << "error: invalid compile option '" << argument << "'\n";
        return 1;
      }
    }

    return CompileFile(argv[2], output.c_str(), options);
  }

  if (command == "dump") {
    if (argc != 4) {
      std::cerr << "usage: fell dump <tokens|ast|ir|bytecode> <file>\n";
      return 1;
    }

    return DumpFile(argv[2], argv[3]);
  }

  std::cerr << "error: unknown command '" << command << "'\n";
  std::cerr << "try 'fell --help' for usage\n";
  return 1;
}

}  // namespace fell::tool

#include <fstream>
#include <iostream>
#include <sstream>

#include "compiler/compiler.h"
#include "core/logger.h"
#include "core/string.h"
#include "runtime/vm.h"

namespace {

int ExecuteCompileResult(const fell::CompileResult& compile_result) {
  if (!compile_result.Succeeded()) {
    for (const fell::Diagnostic& diagnostic : compile_result.diagnostics) {
      std::cerr << "error: " << diagnostic.message << '\n';
    }

    return 1;
  }

  fell::Vm vm{};
  const auto result{vm.Execute(compile_result.program)};

  std::cout << result << '\n';
  return 0;
}

int Execute(fell::StringView source) {
  fell::Compiler compiler{};
  return ExecuteCompileResult(compiler.Compile(source));
}

int ExecuteExpression(fell::StringView source) {
  fell::Compiler compiler{};
  return ExecuteCompileResult(compiler.CompileExpression(source));
}

int RunFile(const char* path) {
  std::ifstream file{path};

  if (!file) {
    fell::Logger::Error("failed to open source file");
    return 1;
  }

  std::ostringstream stream{};
  stream << file.rdbuf();

  return Execute(stream.str());
}

int RunRepl() {
  fell::String line{};

  while (true) {
    std::cout << "fell> ";

    if (!std::getline(std::cin, line)) {
      std::cout << '\n';
      break;
    }

    if (line.empty()) {
      continue;
    }

    ExecuteExpression(line);
  }

  return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
  if (argc == 1) {
    return RunRepl();
  }

  if (argc == 2) {
    return RunFile(argv[1]);
  }

  std::cerr << "usage: fell [file]\n";
  return 1;
}

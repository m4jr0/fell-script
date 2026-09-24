#pragma once

#include "bytecode/bytecode.h"
#include "compiler/ast.h"
#include "core/string.h"
#include "core/vector.h"

namespace fell {

enum class DiagnosticSeverity {
  kError,
  kWarning,
};

struct Diagnostic {
  DiagnosticSeverity severity;
  String message;
};

struct CompileResult {
  BytecodeModule program;
  Vector<Diagnostic> diagnostics;

  bool Succeeded() const;
};

class Compiler {
 public:
  CompileResult Compile(StringView source);
  CompileResult CompileExpression(StringView source);

 private:
  CompileResult CompileUnit(const CompilationUnit& unit);
};

}  // namespace fell

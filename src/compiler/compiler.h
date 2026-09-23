#pragma once

#include "bytecode/bytecode.h"
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
  Program program;
  Vector<Diagnostic> diagnostics;

  bool Succeeded() const;
};

class Compiler {
 public:
  CompileResult Compile(StringView source);
};

}  // namespace fell
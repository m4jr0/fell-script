#pragma once

#include "bytecode/bytecode.h"
#include "compiler/ast.h"
#include "compiler/diagnostic.h"
#include "core/string.h"
#include "core/vector.h"

namespace fell {

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

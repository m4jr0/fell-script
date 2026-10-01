#pragma once

#include "bytecode/bytecode.h"
#include "compiler/ast.h"
#include "compiler/diagnostic.h"
#include "core/string.h"
#include "core/type.h"
#include "core/vector.h"

namespace fell {

enum class CompileDump : u8 {
  kNone = 0,

  kTokens = 1 << 0,
  kAst = 1 << 1,
  kIr = 1 << 2,
  kBytecode = 1 << 3,

  kAll = (1 << 4) - 1,
};

class CompileDumpFlags {
 public:
  [[nodiscard]] constexpr bool Has(CompileDump flag) const {
    return (bits_ & static_cast<u8>(flag)) != 0;
  }

  constexpr void Set(CompileDump flag) { bits_ |= static_cast<u8>(flag); }

  [[nodiscard]] constexpr bool HasSourceDump() const {
    return Has(CompileDump::kTokens) || Has(CompileDump::kAst) ||
           Has(CompileDump::kIr);
  }

 private:
  u8 bits_{0};
};

struct CompileOptions {
  CompileDumpFlags dumps;
};

struct CompileResult {
  BytecodeModule program;
  Vector<Diagnostic> diagnostics;

  String token_dump;
  String ast_dump;
  String ir_dump;
  String bytecode_dump;

  [[nodiscard]] bool Succeeded() const;
};

class Compiler {
 public:
  CompileResult Compile(StringView source, const CompileOptions& options = {});
  CompileResult CompileReplInput(StringView source,
                                 const CompileOptions& options = {});

 private:
  CompileResult CompileUnit(const CompilationUnit& unit,
                            const CompileOptions& options,
                            bool return_last_expression = false);
};

}  // namespace fell

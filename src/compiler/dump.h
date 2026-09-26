#pragma once

#include "compiler/ast.h"
#include "compiler/ir.h"
#include "core/string.h"

namespace fell {

String DumpTokens(StringView source);
String DumpAst(const CompilationUnit& unit);
String DumpIr(const IrProgram& program);

}  // namespace fell

#pragma once

#include "compiler/frontend/ast.h"
#include "compiler/ir/ir.h"
#include "core/string.h"

namespace fell {

String DumpTokens(StringView source);
String DumpAst(const CompilationUnit& unit);
String DumpIr(const IrProgram& program);

}  // namespace fell

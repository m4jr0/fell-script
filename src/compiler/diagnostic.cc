#include "compiler/diagnostic.h"

#include "core/assert.h"

namespace fell {

bool HasErrors(Span<const Diagnostic> diagnostics) {
  for (const Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == DiagnosticSeverity::kError) {
      return true;
    }
  }

  return false;
}

StringView ToString(DiagnosticSeverity severity) {
  switch (severity) {
    case DiagnosticSeverity::kError:
      return "error";
    case DiagnosticSeverity::kWarning:
      return "warning";
  }

  FELL_UNREACHABLE();
}

}  // namespace fell
#pragma once

#include "core/span.h"
#include "core/string.h"

namespace fell {

enum class DiagnosticSeverity {
  kError,
  kWarning,
};

struct Diagnostic {
  DiagnosticSeverity severity;
  String message;
};

bool HasErrors(Span<const Diagnostic> diagnostics);

StringView ToString(DiagnosticSeverity severity);

}  // namespace fell

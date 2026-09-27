#pragma once

#include <optional>

#include "compiler/source_location.h"
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
  std::optional<SourceSpan> span{};
};

bool HasErrors(Span<const Diagnostic> diagnostics);

StringView ToString(DiagnosticSeverity severity);

}  // namespace fell

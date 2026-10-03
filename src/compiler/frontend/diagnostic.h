#pragma once

#include <optional>

#include "compiler/frontend/source_location.h"
#include "core/span.h"
#include "core/string.h"

namespace fell {

enum class DiagnosticSeverity {
  kInvalid,

  kError,
  kWarning,
};

struct Diagnostic {
  DiagnosticSeverity severity{DiagnosticSeverity::kInvalid};
  String message;
  std::optional<SourceSpan> span{};
};

bool HasErrors(Span<const Diagnostic> diagnostics);

StringView ToString(DiagnosticSeverity severity);

}  // namespace fell

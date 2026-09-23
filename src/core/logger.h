#pragma once

#include "core/string.h"

namespace fell {

enum class LogLevel {
  kDebug,
  kInfo,
  kWarning,
  kError,
};

using LogSink = void (*)(LogLevel level, StringView message);

class Logger {
 public:
  static void SetSink(LogSink sink);

  static void Debug(StringView message);
  static void Info(StringView message);
  static void Warning(StringView message);
  static void Error(StringView message);

 private:
  static void Log(LogLevel level, StringView message);
};

}  // namespace fell
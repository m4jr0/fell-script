#pragma once

#include <string_view>

namespace fell {

enum class LogLevel {
  kDebug,
  kInfo,
  kWarning,
  kError,
};

using LogSink = void (*)(LogLevel level, std::string_view message);

class Logger {
 public:
  static void SetSink(LogSink sink);

  static void Debug(std::string_view message);
  static void Info(std::string_view message);
  static void Warning(std::string_view message);
  static void Error(std::string_view message);

 private:
  static void Log(LogLevel level, std::string_view message);
};

}  // namespace fell
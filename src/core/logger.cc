#include "core/logger.h"

#include <iostream>

namespace fell {
namespace {

void DefaultLogSink(LogLevel level, std::string_view message) {
  switch (level) {
    case LogLevel::kDebug:
      std::cerr << "[debug] ";
      break;
    case LogLevel::kInfo:
      std::cerr << "[info] ";
      break;
    case LogLevel::kWarning:
      std::cerr << "[warning] ";
      break;
    case LogLevel::kError:
      std::cerr << "[error] ";
      break;
  }

  std::cerr << message << '\n';
}

LogSink log_sink = &DefaultLogSink;

}  // namespace

void Logger::SetSink(LogSink sink) {
  log_sink = sink != nullptr ? sink : &DefaultLogSink;
}

void Logger::Debug(std::string_view message) {
  Log(LogLevel::kDebug, message);
}

void Logger::Info(std::string_view message) {
  Log(LogLevel::kInfo, message);
}

void Logger::Warning(std::string_view message) {
  Log(LogLevel::kWarning, message);
}

void Logger::Error(std::string_view message) {
  Log(LogLevel::kError, message);
}

void Logger::Log(LogLevel level, std::string_view message) {
  log_sink(level, message);
}

}  // namespace fell
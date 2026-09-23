#include "core/char.h"

#include <cctype>

namespace fell {

bool IsDigit(char character) {
  return std::isdigit(static_cast<unsigned char>(character)) != 0;
}

bool IsWhitespace(char character) {
  return std::isspace(static_cast<unsigned char>(character)) != 0;
}

}  // namespace fell
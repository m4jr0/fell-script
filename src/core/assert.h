#pragma once

#include <cstdlib>

#define FELL_ASSERT(condition) \
  do {                         \
    if (!(condition)) {        \
      std::abort();            \
    }                          \
  } while (false)

#define FELL_UNREACHABLE() \
  do {                     \
    std::abort();          \
  } while (false)

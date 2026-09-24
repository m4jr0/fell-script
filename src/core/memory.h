#pragma once

#include <memory>
#include <utility>

namespace fell {

template <typename T>
using UniquePtr = std::unique_ptr<T>;

template <typename T, typename... Args>
UniquePtr<T> MakeUnique(Args&&... args) {
  return std::make_unique<T>(std::forward<Args>(args)...);
}

}  // namespace fell

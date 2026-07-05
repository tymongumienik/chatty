#pragma once

#include <unistd.h>
#include <functional>
#include <memory>
#include <optional>
#include "lib/unique_resource/unique_resource.hpp"

namespace UniqueFileDescriptor {
struct CloseDeleter {
  void operator()(int fd) const noexcept {
    if (fd >= 0)
      ::close(fd);
  }
};

using Type = std_experimental::unique_resource<int, CloseDeleter>;

inline Type make(int handle) {
  return std_experimental::make_unique_resource(std::move(handle),
                                                CloseDeleter{});
}

template <typename Deleter>
decltype(auto) make(int handle, Deleter&& custom_deleter) {
  return std_experimental::make_unique_resource(
      std::move(handle), std::forward<Deleter>(custom_deleter));
}
}  // namespace UniqueFileDescriptor

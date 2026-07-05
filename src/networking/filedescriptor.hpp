#pragma once

#include <unistd.h>
#include <functional>
#include <memory>
#include <optional>
#include "lib/unique_resource/unique_resource.hpp"

namespace UniqueFileDescriptor {
using Type = std_experimental::unique_resource<int, std::function<void(int)>>;

inline Type make(int handle) {
  return std_experimental::make_unique_resource(
      std::move(handle), std::function<void(int)>([](int fd) {
        if (fd >= 0)
          ::close(fd);
      }));
}

template <typename Deleter>
decltype(auto) make(int handle, Deleter&& custom_deleter) {
  return std_experimental::make_unique_resource(
      std::move(handle), std::forward<Deleter>(custom_deleter));
}
}  // namespace UniqueFileDescriptor

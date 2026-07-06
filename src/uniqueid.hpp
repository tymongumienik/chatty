#pragma once

#include <cstddef>
#include <string>

namespace UniqueId {
constexpr std::size_t length = 64;

std::string make();
}  // namespace UniqueId

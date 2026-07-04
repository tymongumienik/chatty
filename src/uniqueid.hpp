#pragma once

#include <random>

namespace UniqueId {
constexpr size_t length = 64;
inline thread_local std::mt19937 generator(std::random_device{}());
constexpr char chars[] = "0123456789abcdef";

std::string make();
}  // namespace UniqueId

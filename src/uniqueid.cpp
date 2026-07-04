#include "uniqueid.hpp"

std::string UniqueId::make() {
  std::uniform_int_distribution dist(0, 15);

  std::string result{};
  result.reserve(length);  // for good measure

  for (int i = 0; i < length; ++i) {
    result += chars[dist(generator)];
  }

  return result;
}

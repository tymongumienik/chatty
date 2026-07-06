#include "uniqueid.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <array>
#include <stdexcept>
#include "networking/filedescriptor.hpp"

std::string UniqueId::make() {
  constexpr char hex[] = "0123456789abcdef";
  constexpr std::size_t byte_count = length / 2;

  std::array<unsigned char, byte_count> bytes{};

  auto fd = UniqueFileDescriptor::make(::open("/dev/urandom", O_RDONLY));
  if (fd.get() < 0) {
    throw std::runtime_error("Failed to open /dev/urandom");
  }

  ssize_t n = ::read(fd.get(), bytes.data(), bytes.size());

  if (n != static_cast<ssize_t>(bytes.size())) {
    throw std::runtime_error("Failed to read from /dev/urandom");
  }

  std::string result;
  result.reserve(length);
  for (auto b : bytes) {
    result += hex[b >> 4];
    result += hex[b & 0x0F];
  }

  return result;
}

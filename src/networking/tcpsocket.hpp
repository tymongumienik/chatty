#pragma once

#include <cstdint>
#include <string>
#include "filedescriptor.hpp"

namespace Networking {

enum class RecvStatus {
  Data,
  WouldBlock,
  Error,
  Closed,
};

struct RecvResult {
  RecvStatus status;
  std::string data;
};

class TcpSocket {
 public:
  TcpSocket() : fd_(UniqueFileDescriptor::make(-1)) {}
  explicit TcpSocket(UniqueFileDescriptor::Type fd);

  bool connect(const std::string& ip, std::uint16_t port);
  void sendRaw(const std::string& data);
  RecvResult receiveRaw();

  void close();
  bool isValid() const { return fd_.get() >= 0; }

 private:
  UniqueFileDescriptor::Type fd_;
  std::string recv_buffer_;
};

}  // namespace Networking

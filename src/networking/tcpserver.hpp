#pragma once

#include <cstdint>
#include <optional>
#include "filedescriptor.hpp"
#include "tcpsocket.hpp"

namespace Networking {

class TcpServer {
 public:
  explicit TcpServer(std::uint16_t port);

  struct AcceptResult {
    TcpSocket socket;
    std::string peer_address;
  };

  std::optional<AcceptResult> accept();

  void close();

  bool isValid() const { return fd_.get() >= 0; }

  std::uint16_t getPort() const { return port_; }

 private:
  void bindAndListen();

  std::uint16_t port_;
  UniqueFileDescriptor::Type fd_;
};

}  // namespace Networking

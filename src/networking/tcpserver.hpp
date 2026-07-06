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

  std::optional<AcceptResult> Accept();

  void Close();

  bool IsValid() const { return fd_.get() >= 0; }

  std::uint16_t GetPort() const { return port_; }

 private:
  void BindAndListen();

  std::uint16_t port_;
  UniqueFileDescriptor::Type fd_;
};

}  // namespace Networking

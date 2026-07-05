#pragma once
#include <cstdint>
#include <optional>
#include <string>
#include "filedescriptor.hpp"
#include "packets.hpp"

namespace Networking {
class UdpSocket {
 public:
  struct Datagram {
    std::string address;
    std::string data;
  };

  UdpSocket(uint16_t port);

  void sendPacket(const std::string& ip,
                  std::uint16_t target_port,
                  const Networking::Packet& packet);
  std::optional<Datagram> receiveRaw();

 private:
  void bind();
  std::uint16_t port_;
  UniqueFileDescriptor::Type fd_;
};
};  // namespace Networking

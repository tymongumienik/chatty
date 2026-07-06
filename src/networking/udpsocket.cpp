#include "udpsocket.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <stdexcept>
#include "constants.hpp"

Networking::UdpSocket::UdpSocket(std::uint16_t port)
    : port_(port),
      fd_(UniqueFileDescriptor::make(::socket(AF_INET, SOCK_DGRAM, 0))) {
  if (fd_.get() < 0)
    throw std::runtime_error("UDP socket() failed");

  int yes = 1;

  if (::setsockopt(fd_.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
    throw std::runtime_error("UDP setsockopt(SO_REUSEADDR) failed");
  if (::setsockopt(fd_.get(), SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) < 0)
    throw std::runtime_error("UDP setsockopt(SO_REUSEPORT) failed");
  if (::setsockopt(fd_.get(), SOL_SOCKET, SO_BROADCAST, &yes, sizeof(yes)) < 0)
    throw std::runtime_error("UDP setsockopt(SO_BROADCAST) failed");

  int flags = ::fcntl(fd_.get(), F_GETFL);
  if (flags < 0) {
    throw std::runtime_error("UDP fcntl(F_GETFL) failed");
  }
  if (::fcntl(fd_.get(), F_SETFL, flags | O_NONBLOCK) < 0) {
    throw std::runtime_error("UDP set non blocking failed");
  }

  bind();
}

void Networking::UdpSocket::sendPacket(const std::string& ip,
                                       uint16_t target_port,
                                       const Networking::Packet& packet) {
  std::string serialized = packet.serialize();

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(target_port);

  // set address from string
  if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
    throw std::runtime_error("UDP sending packet failed (1)");
  }

  ssize_t result;
  do {
    result = ::sendto(fd_.get(), serialized.data(), serialized.size(), 0,
                      reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  } while (result < 0 && errno == EINTR);

  if (result == -1) {
    throw std::runtime_error("UDP sending packet failed (2)");
  }
}
std::optional<Networking::UdpSocket::Datagram>
Networking::UdpSocket::receiveRaw() {
  char buff[Constants::MAX_PACKET_SIZE]{};

  sockaddr_in sender{};
  socklen_t senderLen = sizeof(sender);

  ssize_t n;
  do {
    n = ::recvfrom(fd_.get(), buff, sizeof(buff), 0,
                   reinterpret_cast<sockaddr*>(&sender), &senderLen);
  } while (n < 0 && errno == EINTR);

  if (n < 0) {
    return std::nullopt;
  }

  char ip[INET_ADDRSTRLEN]{};

  // get address into string
  if (!inet_ntop(AF_INET, &sender.sin_addr, ip, sizeof(ip))) {
    return std::nullopt;
  }

  return Datagram{
      .address = std::string(ip),
      .data = std::string(buff, static_cast<size_t>(n)),
  };
}

void Networking::UdpSocket::bind() {
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_);

  if (::bind(fd_.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    throw std::runtime_error("UDP bind() failed");
  }
}

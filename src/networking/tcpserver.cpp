#include "tcpserver.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdexcept>

namespace Networking {

TcpServer::TcpServer(std::uint16_t port)
    : port_(port),
      fd_(UniqueFileDescriptor::make(::socket(AF_INET, SOCK_STREAM, 0))) {
  if (fd_.get() < 0) {
    throw std::runtime_error("TCP socket() creation failed");
  }

  int yes = 1;
  if (::setsockopt(fd_.get(), SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) <
      0) {
    throw std::runtime_error("TCP setsockopt(SO_REUSEADDR) failed");
  }
  if (::setsockopt(fd_.get(), SOL_SOCKET, SO_REUSEPORT, &yes, sizeof(yes)) <
      0) {
    throw std::runtime_error("TCP setsockopt(SO_REUSEPORT) failed");
  }

  int flags = ::fcntl(fd_.get(), F_GETFL);
  if (flags < 0) {
    throw std::runtime_error("TCP fcntl(F_GETFL) failed");
  }
  if (::fcntl(fd_.get(), F_SETFL, flags | O_NONBLOCK) < 0) {
    throw std::runtime_error("TCP set non-blocking failed");
  }

  BindAndListen();
}

std::optional<TcpServer::AcceptResult> TcpServer::Accept() {
  sockaddr_in peer_addr{};
  socklen_t peer_len = sizeof(peer_addr);

  int client_fd;
  do {
    client_fd =
        ::accept(fd_.get(), reinterpret_cast<sockaddr*>(&peer_addr), &peer_len);
  } while (client_fd < 0 && errno == EINTR);

  if (client_fd < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return std::nullopt;
    }
    return std::nullopt;
  }

  // make the accepted socket non-blocking as well
  int flags = ::fcntl(client_fd, F_GETFL);
  if (flags < 0 || ::fcntl(client_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
    ::close(client_fd);
    return std::nullopt;
  }

  int yes = 1;
  // disable Nagle's algorithm for low-latency chat
  if (::setsockopt(client_fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) <
      0) {
    ::close(client_fd);
    return std::nullopt;
  }
  // enable keepalive to detect dead peers
  if (::setsockopt(client_fd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) <
      0) {
    ::close(client_fd);
    return std::nullopt;
  }

  char ip[INET_ADDRSTRLEN]{};
  ::inet_ntop(AF_INET, &peer_addr.sin_addr, ip, sizeof(ip));

  return AcceptResult{
      TcpSocket(UniqueFileDescriptor::make(client_fd)),
      std::string(ip),
  };
}

void TcpServer::Close() {
  fd_.reset(-1);
}

void TcpServer::BindAndListen() {
  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_);

  if (::bind(fd_.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
    throw std::runtime_error("TCP bind() failed");
  }

  if (::listen(fd_.get(), 10) < 0) {
    throw std::runtime_error("TCP listen() failed");
  }

  if (port_ == 0) {
    socklen_t len = sizeof(addr);
    if (::getsockname(fd_.get(), reinterpret_cast<sockaddr*>(&addr), &len) ==
        0) {
      port_ = ntohs(addr.sin_port);
    }
  }
}

}  // namespace Networking

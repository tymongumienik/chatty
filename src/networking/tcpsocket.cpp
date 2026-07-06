#include "tcpsocket.hpp"
#include <arpa/inet.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <chrono>
#include <cstring>
#include <stdexcept>
#include "constants.hpp"

namespace Networking {

TcpSocket::TcpSocket(UniqueFileDescriptor::Type fd) : fd_(std::move(fd)) {}

bool TcpSocket::connect(const std::string& ip, std::uint16_t port) {
  if (fd_.get() >= 0) {
    close();
  }
  recv_buffer_.clear();

  fd_ = UniqueFileDescriptor::make(::socket(AF_INET, SOCK_STREAM, 0));
  if (fd_.get() < 0) {
    return false;
  }

  int flags = ::fcntl(fd_.get(), F_GETFL);
  if (flags < 0) {
    close();
    return false;
  }
  if (::fcntl(fd_.get(), F_SETFL, flags | O_NONBLOCK) < 0) {
    close();
    return false;
  }

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port);

  if (::inet_pton(AF_INET, ip.c_str(), &addr.sin_addr) != 1) {
    close();
    return false;
  }

  int res =
      ::connect(fd_.get(), reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
  if (res < 0) {
    if (errno == EINPROGRESS || errno == EINTR) {
      pollfd pfd{};
      pfd.fd = fd_.get();
      pfd.events = POLLOUT;

      int poll_res;
      do {
        poll_res = ::poll(&pfd, 1, 2000);
      } while (poll_res < 0 && errno == EINTR);

      if (poll_res > 0) {
        int optval = 0;
        socklen_t optlen = sizeof(optval);
        if (::getsockopt(fd_.get(), SOL_SOCKET, SO_ERROR, &optval, &optlen) <
            0) {
          close();
          return false;
        }
        if (optval != 0) {
          close();
          return false;
        }
      } else {
        close();
        return false;
      }
    } else {
      close();
      return false;
    }
  }

  // disable Nagle's algorithm for low-latency chat
  // enable keepalive to detect dead peers
  int yes = 1;
  if (::setsockopt(fd_.get(), IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes)) <
      0) {
    close();
    return false;
  }
  if (::setsockopt(fd_.get(), SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) <
      0) {
    close();
    return false;
  }

  return true;
}

void TcpSocket::sendRaw(const std::string& data) {
  if (fd_.get() < 0) {
    throw std::runtime_error("TCP socket is not connected");
  }

  if (data.size() > Constants::MAX_MESSAGE_SIZE) {
    throw std::runtime_error("TCP message exceeds maximum size");
  }

  // frame the message: 4-byte big-endian prefix + payload
  std::uint32_t net_len = htonl(static_cast<std::uint32_t>(data.size()));
  std::string frame;
  frame.reserve(Constants::MESSAGE_LENGTH_PREFIX_SIZE + data.size());
  frame.append(reinterpret_cast<const char*>(&net_len),
               Constants::MESSAGE_LENGTH_PREFIX_SIZE);
  frame.append(data);

  constexpr auto timeout = std::chrono::seconds(10);
  auto deadline = std::chrono::steady_clock::now() + timeout;

  size_t total_sent = 0;
  while (total_sent < frame.size()) {
    ssize_t n = ::send(fd_.get(), frame.data() + total_sent,
                       frame.size() - total_sent, MSG_NOSIGNAL);
    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        auto now = std::chrono::steady_clock::now();
        if (now >= deadline) {
          throw std::runtime_error("TCP send timed out");
        }

        auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(
            deadline - now);

        pollfd pfd{};
        pfd.fd = fd_.get();
        pfd.events = POLLOUT;

        int poll_res;
        do {
          poll_res = ::poll(&pfd, 1, static_cast<int>(remaining.count()));
        } while (poll_res < 0 && errno == EINTR);

        if (poll_res <= 0) {
          throw std::runtime_error("TCP send timed out");
        }
        continue;
      }
      throw std::runtime_error("TCP send failed");
    }
    total_sent += static_cast<size_t>(n);
  }
}

RecvResult TcpSocket::receiveRaw() {
  if (fd_.get() < 0) {
    return {RecvStatus::Error, {}};
  }

  // First, check if we already have a complete message in the buffer
  if (recv_buffer_.size() >= Constants::MESSAGE_LENGTH_PREFIX_SIZE) {
    std::uint32_t net_len;
    std::memcpy(&net_len, recv_buffer_.data(),
                Constants::MESSAGE_LENGTH_PREFIX_SIZE);
    std::uint32_t msg_len = ntohl(net_len);

    if (msg_len > Constants::MAX_MESSAGE_SIZE) {
      return {RecvStatus::Error, {}};
    }

    std::size_t frame_size = Constants::MESSAGE_LENGTH_PREFIX_SIZE + msg_len;
    if (recv_buffer_.size() >= frame_size) {
      std::string message =
          recv_buffer_.substr(Constants::MESSAGE_LENGTH_PREFIX_SIZE, msg_len);
      recv_buffer_.erase(0, frame_size);
      return {RecvStatus::Data, std::move(message)};
    }
  }

  // If not, read any available data
  char buff[Constants::MAX_PACKET_SIZE];
  ssize_t n;
  do {
    n = ::recv(fd_.get(), buff, sizeof(buff), 0);
  } while (n < 0 && errno == EINTR);

  if (n > 0) {
    recv_buffer_.append(buff, static_cast<size_t>(n));
  } else if (n == 0) {
    return {RecvStatus::Closed, {}};
  } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
    return {RecvStatus::Error, {}};
  }

  // Check again after appending new data
  if (recv_buffer_.size() < Constants::MESSAGE_LENGTH_PREFIX_SIZE) {
    return {RecvStatus::WouldBlock, {}};
  }

  std::uint32_t net_len;
  std::memcpy(&net_len, recv_buffer_.data(),
              Constants::MESSAGE_LENGTH_PREFIX_SIZE);
  std::uint32_t msg_len = ntohl(net_len);

  if (msg_len > Constants::MAX_MESSAGE_SIZE) {
    return {RecvStatus::Error, {}};
  }

  std::size_t frame_size = Constants::MESSAGE_LENGTH_PREFIX_SIZE + msg_len;
  if (recv_buffer_.size() < frame_size) {
    return {RecvStatus::WouldBlock, {}};
  }

  // trim out the prefix now that it's not useful anymore
  std::string message =
      recv_buffer_.substr(Constants::MESSAGE_LENGTH_PREFIX_SIZE, msg_len);
  recv_buffer_.erase(0, frame_size);

  return {RecvStatus::Data, std::move(message)};
}

void TcpSocket::close() {
  fd_.reset();
  recv_buffer_.clear();
}

}  // namespace Networking

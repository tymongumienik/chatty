#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <string>
#include <thread>
#include "tcpserver.hpp"
#include "tcpsocket.hpp"
#include "udpsocket.hpp"

namespace Networking {
struct Interop {
  std::function<void(std::string)> on_message;
  std::function<void(std::string)> on_peer_found;
  std::function<void()> on_peer_disconnected;
};

struct Command {
  enum class Type {
    SearchPeer,
    SendMessage,
    Stop,
  };

  Type type;
  std::string payload;
};

enum class ClientStage {
  Idle,
  SearchingForPeer,
  Chatting,
};

enum class ClientRole {
  None,
  Hosting,
  Connecting,
};

class Client {
 public:
  explicit Client(Interop interop);
  ~Client();

  void Start();
  void Stop();

  void SearchPeer(std::string username);
  void SendMessage(std::string text);

  std::string GetPeerIp() const {
    std::lock_guard lock(data_mutex_);
    return peer_ip_;
  }
  std::string GetPeerUsername() const {
    std::lock_guard lock(data_mutex_);
    return peer_username_;
  }
  std::uint16_t GetPort() const { return peer_tcp_port_.load(); }
  ClientRole GetRole() const { return role_.load(); }

 private:
  void NetworkLoop(std::stop_token st);

  // handshake
  void BroadcastDiscovery(
      UdpSocket& discovery_socket,
      std::chrono::steady_clock::time_point& last_discover_sent);
  void ProcessDiscoveryPackets(std::optional<UdpSocket>& discovery_socket);

  // we are the host
  void StartHostingSession(const std::string& peer_ip,
                           UdpSocket& discovery_socket,
                           const std::string& remote_username);
  bool PollIncomingConnection(std::optional<UdpSocket>& discovery_socket);

  // the peer is the host
  bool ConnectToPeerHost(const std::string& peer_ip,
                         std::uint16_t remote_port,
                         std::optional<UdpSocket>& discovery_socket,
                         const std::string& remote_username);

  void HandlePeerDisconnected();

  Interop interop_;
  std::jthread network_thread_;

  std::mutex commands_mutex_;
  std::queue<Command> commands_;

  mutable std::mutex data_mutex_;

  ClientStage stage_ = ClientStage::Idle;
  std::atomic<ClientRole> role_ = ClientRole::None;
  std::string username_;
  std::string instance_id_;
  std::string peer_ip_;
  std::string peer_username_;
  std::atomic<std::uint16_t> peer_tcp_port_ = 0;

  std::optional<TcpServer> tcp_server_;
  std::optional<TcpSocket> tcp_socket_;
};
}  // namespace Networking

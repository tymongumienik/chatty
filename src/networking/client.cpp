#include "client.hpp"
#include <chrono>
#include <string_view>
#include "constants.hpp"
#include "packets.hpp"
#include "udpsocket.hpp"
#include "uniqueid.hpp"

namespace Networking {
Client::Client(Interop interop)
    : interop_(std::move(interop)), instance_id_(UniqueId::make()) {}

Client::~Client() {
  Stop();
}

void Client::Start() {
  network_thread_ =
      std::jthread([this](std::stop_token st) { NetworkLoop(st); });
}

void Client::Stop() {
  network_thread_ =
      {};  // move assignment will take care of deinitializing old jthread

  if (tcp_server_) {
    tcp_server_->close();
    tcp_server_.reset();
  }
  if (tcp_socket_) {
    tcp_socket_->close();
    tcp_socket_.reset();
  }
  role_ = ClientRole::None;
  stage_ = ClientStage::Idle;
}

void Client::SearchPeer(std::string username) {
  std::lock_guard lock(commands_mutex_);
  commands_.push(Command{Command::Type::SearchPeer, std::move(username)});
}

void Client::SendMessage(std::string text) {
  std::lock_guard lock(commands_mutex_);
  commands_.push(Command{Command::Type::SendMessage, std::move(text)});
}

void Client::NetworkLoop(std::stop_token st) {
  std::optional<UdpSocket> discovery_socket;
  auto last_discover_sent = std::chrono::steady_clock::time_point{};

  while (!st.stop_requested()) {
    {
      std::lock_guard lock(commands_mutex_);
      while (!commands_.empty()) {
        auto command = std::move(commands_.front());
        commands_.pop();

        if (command.type == Command::Type::SearchPeer) {
          username_ = std::move(command.payload);
          stage_ = ClientStage::SearchingForPeer;
          role_ = ClientRole::None;

          if (!discovery_socket) {
            discovery_socket.emplace(Constants::DISCOVERY_PORT);
          }
        }

        if (command.type == Command::Type::SendMessage) {
          if (tcp_socket_ && tcp_socket_->isValid()) {
            try {
              tcp_socket_->sendRaw(command.payload);
            } catch (const std::exception&) {
              HandlePeerDisconnected();
            }
          }
        }

        if (command.type == Command::Type::Stop) {
          stage_ = ClientStage::Idle;
          role_ = ClientRole::None;
          if (discovery_socket) {
            discovery_socket.reset();
          }
          if (tcp_server_) {
            tcp_server_->close();
            tcp_server_.reset();
          }
          if (tcp_socket_) {
            tcp_socket_->close();
            tcp_socket_.reset();
          }
        }
      }
    }

    if (stage_ == ClientStage::SearchingForPeer && discovery_socket) {
      // we broadcast that we are here and ready to host a connection
      if (role_ == ClientRole::None) {
        BroadcastDiscovery(*discovery_socket, last_discover_sent);
      }

      // check if another peer connected to our server
      if (PollIncomingConnection(discovery_socket)) {
        continue;
      }

      // we get a response from another peer
      ProcessDiscoveryPackets(discovery_socket);
    }

    if (stage_ == ClientStage::Chatting && tcp_socket_) {
      auto result = tcp_socket_->receiveRaw();
      switch (result.status) {
        case RecvStatus::Data:
          interop_.on_message(std::move(result.data));
          break;
        case RecvStatus::Closed:
        case RecvStatus::Error:
          HandlePeerDisconnected();
          break;
        case RecvStatus::WouldBlock:
          break;
      }
    }

    std::this_thread::sleep_for(
        std::chrono::milliseconds(Constants::NETWORK_POLL_INTERVAL_MS));
  }
}

void Client::BroadcastDiscovery(
    UdpSocket& discovery_socket,
    std::chrono::steady_clock::time_point& last_discover_sent) {
  auto now = std::chrono::steady_clock::now();

  if (now - last_discover_sent >= std::chrono::seconds(1)) {
    DiscoverPacket packet;
    packet.instanceId = instance_id_;
    packet.tcpPort = 0;  // 0 signifies "You host"
    packet.username = username_;
    discovery_socket.sendPacket(std::string(Constants::DISCOVERY_ADDRESS),
                                Constants::DISCOVERY_PORT, packet);
    last_discover_sent = now;
  }
}

void Client::ProcessDiscoveryPackets(
    std::optional<UdpSocket>& discovery_socket) {
  while (discovery_socket) {
    auto datagram = discovery_socket->receiveRaw();
    if (!datagram) {
      break;
    }

    auto packet = Packet::parse(datagram->data);
    if (!packet)
      continue;

    std::string remote_instance_id;
    std::uint16_t remote_tcp_port = 0;
    std::string remote_username;

    const bool is_discover =
        dynamic_cast<DiscoverPacket*>(packet.get()) != nullptr;

    if (auto* discover = dynamic_cast<DiscoverPacket*>(packet.get())) {
      remote_instance_id = discover->instanceId;
      remote_tcp_port = discover->tcpPort;
      remote_username = discover->username;
    } else if (auto* hello = dynamic_cast<HelloPacket*>(packet.get())) {
      remote_instance_id = hello->instanceId;
      remote_tcp_port = hello->tcpPort;
      remote_username = hello->username;
    }

    if (remote_instance_id.empty() || remote_instance_id == instance_id_) {
      continue;
    }

    // tiebreaker: the peer with the smaller instance id hosts
    if (instance_id_ < remote_instance_id) {
      // WE HOST
      if (role_ == ClientRole::None) {
        StartHostingSession(datagram->address, *discovery_socket,
                            remote_username);
      } else if (role_ == ClientRole::Hosting && is_discover) {
        StartHostingSession(datagram->address, *discovery_socket,
                            remote_username);
      }
    } else {
      // THEY HOST
      if (remote_tcp_port > 0) {
        if (ConnectToPeerHost(datagram->address, remote_tcp_port,
                              discovery_socket, remote_username)) {
          break;
        }
      } else {
        // send hello packet to wake up the other server for them to host for us
        HelloPacket hello;
        hello.instanceId = instance_id_;
        hello.tcpPort = 0;
        hello.username = username_;
        discovery_socket->sendPacket(std::string(Constants::DISCOVERY_ADDRESS),
                                     Constants::DISCOVERY_PORT, hello);
      }
    }
  }
}

void Client::StartHostingSession(const std::string& peer_ip,
                                 UdpSocket& discovery_socket,
                                 const std::string& remote_username) {
  role_ = ClientRole::Hosting;
  {
    std::lock_guard lock(data_mutex_);
    peer_ip_ = peer_ip;
    peer_username_ = remote_username;
  }

  if (!tcp_server_) {
    tcp_server_.emplace(0);  // automatic port
  }

  peer_tcp_port_ = tcp_server_->getPort();

  HelloPacket hello;
  hello.instanceId = instance_id_;
  hello.tcpPort = tcp_server_->getPort();
  hello.username = username_;
  discovery_socket.sendPacket(std::string(Constants::DISCOVERY_ADDRESS),
                              Constants::DISCOVERY_PORT, hello);
}

bool Client::PollIncomingConnection(
    std::optional<UdpSocket>& discovery_socket) {
  if (!tcp_server_) {
    return false;
  }

  if (auto accepted = tcp_server_->accept()) {
    // validate that the connecting peer matches the expected address
    std::string expected_ip;
    {
      std::lock_guard lock(data_mutex_);
      expected_ip = peer_ip_;
    }
    if (!expected_ip.empty() && accepted->peer_address != expected_ip) {
      return false;  // reject connection from unexpected peer
    }

    tcp_socket_ = std::move(accepted->socket);
    stage_ = ClientStage::Chatting;

    std::string peer_username;
    {
      std::lock_guard lock(data_mutex_);
      peer_username = peer_username_;
    }

    // close the server socket (we connect to the other peer)
    tcp_server_->close();
    tcp_server_.reset();

    discovery_socket.reset();
    interop_.on_peer_found(peer_username);
    return true;
  }

  return false;
}

void Client::HandlePeerDisconnected() {
  if (tcp_socket_) {
    tcp_socket_->close();
    tcp_socket_.reset();
  }

  if (tcp_server_) {
    tcp_server_->close();
    tcp_server_.reset();
  }

  {
    std::lock_guard lock(data_mutex_);
    peer_ip_.clear();
    peer_username_.clear();
  }

  peer_tcp_port_ = 0;
  stage_ = ClientStage::Idle;
  role_ = ClientRole::None;

  if (interop_.on_peer_disconnected) {
    interop_.on_peer_disconnected();
  }
}

bool Client::ConnectToPeerHost(const std::string& peer_ip,
                               std::uint16_t remote_port,
                               std::optional<UdpSocket>& discovery_socket,
                               const std::string& remote_username) {
  role_ = ClientRole::Connecting;
  {
    std::lock_guard lock(data_mutex_);
    peer_ip_ = peer_ip;
    peer_username_ = remote_username;
  }
  peer_tcp_port_ = remote_port;

  // initiate TCP connection to the responder's hosting port
  tcp_socket_.emplace();
  if (tcp_socket_->connect(peer_ip, peer_tcp_port_)) {
    stage_ = ClientStage::Chatting;
    discovery_socket.reset();
    interop_.on_peer_found(remote_username);
    return true;
  }

  tcp_socket_.reset();
  role_ = ClientRole::None;
  return false;
}

}  // namespace Networking

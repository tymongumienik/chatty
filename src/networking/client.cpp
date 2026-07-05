#include "client.hpp"
#include <chrono>
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
  if (network_thread_.joinable()) {
    network_thread_.request_stop();
  }
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

          if (!discovery_socket) {
            discovery_socket.emplace(Constants::DISCOVERY_PORT);
          }
        }

        if (command.type == Command::Type::SendMessage) {
          // TODO
        }
      }
    }

    if (stage_ == ClientStage::SearchingForPeer && discovery_socket) {
      auto now = std::chrono::steady_clock::now();

      if (now - last_discover_sent >= std::chrono::seconds(1)) {
        DiscoverPacket packet;
        packet.instanceId = instance_id_;
        packet.tcpPort = 0;  // TODO: set actual TCP port
        discovery_socket->sendPacket(Constants::DISCOVERY_ADDRESS,
                                     Constants::DISCOVERY_PORT, packet);
        last_discover_sent = now;
      }

      // listen for incoming packets
      while (auto datagram = discovery_socket->receiveRaw()) {
        auto packet = Packet::parse(datagram->data);
        if (!packet)
          continue;

        auto transition_to_chat = [&] {
          stage_ = ClientStage::Chatting;
          discovery_socket.reset();
          interop_.on_peer_found();
        };

        // another instance is looking for peers - respond with hello
        if (auto* discover = dynamic_cast<DiscoverPacket*>(packet.get())) {
          if (discover->instanceId != instance_id_) {
            HelloPacket hello;
            hello.instanceId = instance_id_;
            hello.tcpPort = 0;  // TODO: set actual TCP port
            discovery_socket->sendPacket(datagram->address,
                                         Constants::DISCOVERY_PORT, hello);
            peer_tcp_port_ = discover->tcpPort;
            transition_to_chat();
            break;
          }
        }

        // a response to our discover
        if (auto* hello = dynamic_cast<HelloPacket*>(packet.get())) {
          if (hello->instanceId != instance_id_) {
            peer_tcp_port_ = hello->tcpPort;
            transition_to_chat();
            break;
          }
        }
      }
    }

    if (stage_ == ClientStage::Chatting) {
      // TODO
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(10));
  }
}

}  // namespace Networking

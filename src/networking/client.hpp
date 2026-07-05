#pragma once

#include <functional>
#include <mutex>
#include <optional>
#include <queue>
#include <stop_token>
#include <string>
#include <thread>

namespace Networking {
struct Interop {
  std::function<void()> on_message;
  std::function<void()> on_peer_found;
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

class Client {
 public:
  explicit Client(Interop interop);
  ~Client();

  void Start();
  void Stop();

  void SearchPeer(std::string username);
  void SendMessage(std::string text);

 private:
  void NetworkLoop(std::stop_token st);

  Interop interop_;
  std::jthread network_thread_;

  std::mutex commands_mutex_;
  std::queue<Command> commands_;

  ClientStage stage_ = ClientStage::Idle;
  std::string username_;
  std::string instance_id_;
  std::string peer_address_;
};
}  // namespace Networking

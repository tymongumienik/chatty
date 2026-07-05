#pragma once

#include <functional>
#include <stop_token>
#include <string>
#include <thread>
#include <mutex>
#include <queue>

namespace Networking {
struct Interop {
  std::function<void()> on_message;
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
};
}  // namespace Networking

#include "client.hpp"

namespace Networking {
Client::Client(Interop interop) : interop_(std::move(interop)) {}
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
  while (!st.stop_requested()) {
    Command command;
    bool has_command = false;

    {
      std::lock_guard lock(commands_mutex_);

      if (!commands_.empty()) {
        command = std::move(commands_.front());
        commands_.pop();
        has_command = true;
      }
    }

    if (!has_command) {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      continue;
    }

    if (command.type == Command::Type::SearchPeer) {
      // search peer using command.payload
    }

    if (command.type == Command::Type::SendMessage) {
      // send command.payload
    }
  }
}

};  // namespace Networking

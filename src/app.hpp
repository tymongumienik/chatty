#pragma once

#include <mutex>
#include <string>
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"
#include "message.hpp"
#include "networking/client.hpp"

enum class AppStage {
  NameInput,
  WaitingForPeer,
  Chatting,
};

struct AppState {
  AppStage stage = AppStage::NameInput;

  std::string username;
  std::vector<Message> messages{};
};

class App {
 public:
  App();
  void Run();

 private:
  ftxui::ScreenInteractive screen_;
  AppState state_;
  int tab_index_ = 0;  // same order as in AppState struct
  Networking::Client network_;
  std::mutex state_mutex_;

  ftxui::Component MakeNameInputScreen();
  ftxui::Component MakeWaitingForPeerScreen();
  ftxui::Component MakeChatScreen();

  void SetStage(AppStage stage);
};

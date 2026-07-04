#pragma once

#include <string>
#include "ftxui/component/component.hpp"
#include "ftxui/component/screen_interactive.hpp"

enum class AppStage {
  NameInput,
  WaitingForPeer,
  Chatting,
};

struct AppState {
  AppStage stage = AppStage::NameInput;

  std::string username;
};

class App {
 public:
  App();
  void Run();

 private:
  ftxui::ScreenInteractive screen_;
  AppState state_;
  int tab_index_ = 0;  // same order as in AppState struct

  ftxui::Component MakeNameInputScreen();
  ftxui::Component MakeWaitingForPeerScreen();
  ftxui::Component MakeChatScreen();

  void SetStage(AppStage stage);
};

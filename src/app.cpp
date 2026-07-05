#include "app.hpp"
#include "networking/client.hpp"
#include "networking/filedescriptor.hpp"

App::App()
    : screen_(ftxui::ScreenInteractive::Fullscreen()),
      network_({.on_message = [this]() {
        screen_.PostEvent(ftxui::Event::Custom);
      }}) {}

void App::SetStage(AppStage stage) {
  state_.stage = stage;
  tab_index_ = static_cast<int>(stage);
}

void App::Run() {
  using namespace ftxui;

  auto screens = Container::Tab(
      {
          MakeNameInputScreen(),
          MakeWaitingForPeerScreen(),
          MakeChatScreen(),
      },
      &tab_index_);

  screen_.Loop(screens);
}

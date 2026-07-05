#include "app.hpp"
#include "networking/client.hpp"
#include "networking/filedescriptor.hpp"

App::App()
    : screen_(ftxui::ScreenInteractive::Fullscreen()),
      network_(
          {.on_message = [this]() { screen_.PostEvent(ftxui::Event::Custom); },
           .on_peer_found =
               [this]() {
                 {
                   std::lock_guard lock(state_mutex_);
                   state_.stage = AppStage::Chatting;
                   tab_index_ = static_cast<int>(AppStage::Chatting);
                 }
                 screen_.PostEvent(ftxui::Event::Custom);
               }}) {
  network_.Start();
}

void App::SetStage(AppStage stage) {
  if (stage == AppStage::WaitingForPeer) {
    network_.SearchPeer(state_.username);
  }
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

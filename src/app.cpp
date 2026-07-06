#include "app.hpp"
#include "networking/client.hpp"

App::App()
    : screen_(ftxui::ScreenInteractive::Fullscreen()),
      network_({.on_message =
                    [this](std::string text) {
                      std::lock_guard lock(state_mutex_);

                      state_.messages.push_back({
                          .sender = state_.peer_username,
                          .text = std::move(text),
                          .mine = false,
                      });

                      screen_.PostEvent(ftxui::Event::Custom);
                    },
                .on_peer_found =
                    [this](std::string peer_username) {
                      {
                        std::lock_guard lock(state_mutex_);
                        state_.peer_username = std::move(peer_username);
                        state_.stage = AppStage::Chatting;
                        tab_index_ = static_cast<int>(AppStage::Chatting);
                      }
                      screen_.PostEvent(ftxui::Event::Custom);
                    },
                .on_peer_disconnected =
                    [this]() { ResetAfterPeerDisconnected(); }}) {
  network_.Start();
}

void App::SetStage(AppStage stage) {
  std::string username_to_search;
  {
    std::lock_guard lock(state_mutex_);
    if (stage == AppStage::WaitingForPeer) {
      username_to_search = state_.username;
    }
    state_.stage = stage;
    tab_index_ = static_cast<int>(stage);
  }

  if (!username_to_search.empty()) {
    network_.SearchPeer(username_to_search);
  }
}

void App::ResetAfterPeerDisconnected() {
  {
    std::lock_guard lock(state_mutex_);
    state_.messages.clear();
    state_.peer_username.clear();
    state_.stage = AppStage::NameInput;
    tab_index_ = static_cast<int>(AppStage::NameInput);
  }

  screen_.PostEvent(ftxui::Event::Custom);
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

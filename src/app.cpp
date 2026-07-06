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
                      }
                      screen_.PostEvent(ftxui::Event::Custom);
                    },
                .on_peer_disconnected =
                    [this]() { ResetAfterPeerDisconnected(); }}) {
  network_.Start();
}

void App::SetStage(AppStage stage) {
  std::string username_to_search;
  bool should_cancel = false;
  {
    std::lock_guard lock(state_mutex_);
    if (state_.stage == AppStage::WaitingForPeer &&
        stage == AppStage::NameInput) {
      should_cancel = true;
    }
    if (stage == AppStage::WaitingForPeer) {
      username_to_search = state_.username;
    }
    state_.stage = stage;
  }

  if (should_cancel) {
    network_.Cancel();
  }

  if (!username_to_search.empty()) {
    network_.SearchPeer(username_to_search);
  }

  screen_.PostEvent(ftxui::Event::Custom);
}

void App::ResetAfterPeerDisconnected() {
  {
    std::lock_guard lock(state_mutex_);
    state_.messages.clear();
    state_.peer_username.clear();
    state_.stage = AppStage::NameInput;
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

  auto event_handler = CatchEvent(screens, [this](Event e) {
    if (e == Event::Custom) {
      std::lock_guard lock(state_mutex_);
      tab_index_ = static_cast<int>(state_.stage);
    }
    return false;
  });

  screen_.Loop(event_handler);
}

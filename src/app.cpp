#include "app.hpp"

App::App() : screen_(ftxui::ScreenInteractive::Fullscreen()) {}

void App::SetStage(AppStage stage) {
    state_.stage = stage;
    tab_index_ = static_cast<int>(stage);
}

void App::Run() {
    using namespace ftxui;

    auto screens = Container::Tab({
        MakeNameInputScreen(),
        MakeWaitingForPeerScreen(),
        MakeChatScreen(),
    }, &tab_index_);

    screen_.Loop(screens);
}

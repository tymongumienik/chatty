#include "ftxui/screen/color.hpp"

#include "app.hpp"
#include "constants.hpp"

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

ftxui::Component App::MakeNameInputScreen() {
    using namespace ftxui;

    auto input = Input(&state_.username, "username", InputOption{
        .transform = [](InputState s) {
            return s.element | bgcolor(Color::Black) | color(Color::White);
        },
        .multiline = false,
        .on_enter = [this] { SetStage(AppStage::WaitingForPeer); },
    });

    auto container = Container::Vertical({input});

    return Renderer(container, [this, input] {
        Elements art_nodes;
        for (const auto& line : ASCII_ART) {
            art_nodes.push_back(text(std::string(line)));
        }

        return center(vbox({
            color(Color::Green, vbox(art_nodes)) | hcenter,
            filler(),
            text("Enter your username and press ENTER to confirm"),
            text(""),
            input->Render() | size(WIDTH, EQUAL, 35) | hcenter,
        }) | flex);
    });
}

ftxui::Component App::MakeWaitingForPeerScreen() {
    using namespace ftxui;

    auto back_btn = Button("Back", [this] { SetStage(AppStage::NameInput); });

    auto container = Container::Vertical({back_btn});

    return Renderer(container, [this, back_btn] {
        return center(vbox({
            text("Connected as: " + state_.username),
            filler(),
            text("Waiting for a peer to connect..."), // not implemented yet :)
            filler(),
            back_btn->Render(),
        }));
    });
}

ftxui::Component App::MakeChatScreen() {
    using namespace ftxui;

    auto exit_btn = Button("Exit", [this] { screen_.ExitLoopClosure()(); });

    auto container = Container::Vertical({exit_btn});

    return Renderer(container, [this, exit_btn] {
        return vbox({
            hbox({filler(), text("Welcome to Chatty, " + state_.username + "!"), filler()}),
            filler(),
            hbox({filler(), exit_btn->Render(), filler()}),
        });
    });
}

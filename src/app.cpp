#include <memory>

#include "ftxui/component/component_base.hpp"
#include "ftxui/dom/elements.hpp"
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
    auto skip_btn = Button("Skip", [this] { SetStage(AppStage::Chatting); });

    auto container = Container::Vertical({back_btn, skip_btn});

    return Renderer(container, [this, back_btn, skip_btn] {
        return center(vbox({
            text("Connected as: " + state_.username),
            filler(),
            text("Waiting for a peer to connect..."), // not implemented yet :)
            filler(),
            back_btn->Render(),
            skip_btn->Render(),
        }));
    });
}

ftxui::Component App::MakeChatScreen() {
    using namespace ftxui;

    struct Message {
        std::string user;
        std::string text;
        bool mine;
    };

    // solution to scrollable table from https://github.com/ArthurSonzogni/FTXUI/discussions/757#discussioncomment-9760396, adjusted for v7.0.0 (OnRender instead of Render)
    class MessageComponent : public ComponentBase {
    public:
        MessageComponent(Message msg) : msg_(msg) {}

    private:
        Element OnRender() final {
            auto bubble =
                vbox(Elements{
                    text(msg_.user) | bold,
                    paragraph(msg_.text),
                }) |
                borderRounded |
                size(WIDTH, LESS_THAN, 50);

            if (Focused()) {
                bubble = focus(bubble);
            } else if (Active()) {
                bubble = select(bubble);
            }

            return msg_.mine
                ? hbox(Elements{filler(), bubble | bgcolor(Color::Blue)}) // align right 
                : hbox(Elements{bubble | bgcolor(Color::GrayDark), filler()}); // align left 
        }

        bool Focusable() const final { return true; }

        Message msg_;
    };

    auto messages_container = Container::Vertical({});

    messages_container->Add(Make<MessageComponent>(Message{"Janusz", "Test", false}));
    messages_container->Add(Make<MessageComponent>(Message{"You", "It works yay", true}));

    auto input_text = std::make_shared<std::string>();
    auto input = Input(input_text.get(), InputOption { .placeholder = "Type a message..." });

    auto send_message = [messages_container, input_text] {
        if (input_text->empty())
            return;
        messages_container->Add(Make<MessageComponent>(Message{"You", *input_text, true}));
        input_text->clear();
    };

    auto send_btn = Button("Send", send_message);
    auto exit_btn = Button("Exit", [this] {
        screen_.ExitLoopClosure()();
    });

    auto bottom_bar = Container::Horizontal({
        input,
        send_btn,
        exit_btn,
    });

    auto container = Container::Vertical({
        messages_container,
        bottom_bar,
    });

    return CatchEvent(
        Renderer(container, [this, input, send_btn, exit_btn, messages_container] {
            return vbox({
                hbox({
                    filler(),
                    text(std::format("Welcome to Chatty, {}!", state_.username)) | bold,
                    filler(),
                }),
                separator(),
                messages_container->Render() | vscroll_indicator | frame | flex,
                hbox({
                    text(" "),
                    input->Render() | flex,
                    text(" "),
                    send_btn->Render(),
                    text(" "),
                    exit_btn->Render(),
                    text(" "),
                }) | border,
            });
        }),
        [send_message](Event event) {
            if (event == Event::Return) {
                send_message();
                return true;
            }
            return false;
        }
    );
}

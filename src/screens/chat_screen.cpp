#include <memory>
#include <string>

#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"

#include "app.hpp"

ftxui::Component App::MakeChatScreen() {
    using namespace ftxui;

    struct Message {
        std::string user;
        std::string text;
        bool mine;
    };

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
                ? hbox(Elements{filler(), bubble | bgcolor(Color::Blue)})
                : hbox(Elements{bubble | bgcolor(Color::GrayDark), filler()});
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
        messages_container->SetActiveChild(messages_container->ChildAt(messages_container->ChildCount() - 1));
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

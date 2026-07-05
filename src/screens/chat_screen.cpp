#include <format>
#include <memory>
#include <string>
#include <vector>

#include "ftxui/component/component.hpp"
#include "ftxui/component/event.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"

#include "app.hpp"

ftxui::Component App::MakeChatScreen() {
  using namespace ftxui;

  class MessageComponent : public ComponentBase {
   public:
    MessageComponent(Message msg) : msg_(std::move(msg)) {}

   private:
    Element OnRender() final {
      auto bubble = vbox({
                        text(msg_.sender) | bold,
                        paragraph(msg_.text),
                    }) |
                    borderRounded | size(WIDTH, LESS_THAN, 50);

      if (Focused()) {
        bubble = focus(bubble);
      } else if (Active()) {
        bubble = select(bubble);
      }

      return msg_.mine ? hbox({filler(), bubble | bgcolor(Color::Blue)})
                       : hbox({bubble | bgcolor(Color::GrayDark), filler()});
    }

    bool Focusable() const final { return true; }
    const Message msg_;
  };

  auto messages_container = Container::Vertical({});

  auto input_text = std::make_shared<std::string>();
  auto input = Input(input_text.get(),
                     InputOption{.placeholder = "Type a message...",
                                 .transform = [](InputState s) {
                                   return s.element | bgcolor(Color::Default);
                                 }});

  auto append_message_to_ui = [messages_container](Message msg) {
    messages_container->Add(Make<MessageComponent>(std::move(msg)));

    if (messages_container->ChildCount() > 0) {
      messages_container->SetActiveChild(
          messages_container->ChildAt(messages_container->ChildCount() - 1));
    }
  };

  auto send_message = [this, append_message_to_ui, input_text] {
    if (input_text->empty())
      return;

    // TODO

    input_text->clear();
  };

  auto send_btn = Button("Send", send_message);
  auto exit_btn = Button("Exit", [this] { screen_.ExitLoopClosure()(); });
  auto bottom_bar = Container::Horizontal({input, send_btn, exit_btn});

  auto main_container = Container::Vertical({messages_container, bottom_bar});
  main_container->SetActiveChild(bottom_bar);

  return CatchEvent(
      Renderer(
          main_container,
          [messages_container, bottom_bar, this] {
            std::string username{};
            {
              std::lock_guard lock(state_mutex_);
              username = state_.username;
            }

            return vbox({
                hbox({
                    filler(),
                    text(std::format("Welcome to Chatty, {}!", username)) |
                        bold,
                    filler(),
                }),
                separator(),
                messages_container->Render() | vscroll_indicator | frame | flex,
                bottom_bar->Render() | border,
            });
          }),
      [this, send_message, input, append_message_to_ui,
       messages_container](Event event) {
        if (event == Event::Return && input->Focused()) {
          send_message();
          return true;
        }

        if (event == Event::Custom) {
          std::lock_guard lock(state_mutex_);

          while (messages_container->ChildCount() < state_.messages.size()) {
            size_t index = messages_container->ChildCount();
            append_message_to_ui(state_.messages[index]);
          }

          return true;
        }

        return false;
      });
}

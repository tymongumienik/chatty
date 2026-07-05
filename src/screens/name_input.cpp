#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"
#include "ftxui/screen/color.hpp"

#include <memory>

#include "app.hpp"
#include "constants.hpp"

ftxui::Component App::MakeNameInputScreen() {
  using namespace ftxui;

  auto ui_username = std::make_shared<std::string>();

  auto input = Input(ui_username.get(), "username",
                     InputOption{
                         .transform =
                             [](InputState s) {
                               return s.element | bgcolor(Color::Default) |
                                      color(Color::White);
                             },
                         .multiline = false,
                         .on_change =
                             [this, ui_username] {
                               std::lock_guard<std::mutex> lock(state_mutex_);
                               state_.username = *ui_username;
                             },
                         .on_enter =
                             [this, ui_username] {
                               {
                                 std::lock_guard<std::mutex> lock(state_mutex_);
                                 state_.username = *ui_username;
                               }
                               SetStage(AppStage::WaitingForPeer);
                             },
                     });

  auto container = Container::Vertical({input});

  return Renderer(container, [this, input] {
    Elements art_nodes;
    for (const auto& line : Constants::ASCII_ART) {
      art_nodes.push_back(text(std::string(line)));
    }

    return center(vbox({
                      color(Color::Green, vbox(art_nodes)) | hcenter,
                      filler(),
                      text("Enter your username and press ENTER to confirm"),
                      text(""),
                      input->Render() | size(WIDTH, EQUAL, 35) | hcenter,
                  }) |
                  flex);
  });
}

#include "ftxui/component/component.hpp" 
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

#include "constants.hpp"

int main() {
  using namespace ftxui;

  auto screen = ScreenInteractive::Fullscreen();

  std::string name;
  bool joined = false;

  InputOption input_opts {
    .multiline = false,
    .on_enter = [&] { joined = true; },
  };

  auto name_input = Input(&name, "username", input_opts);
  auto exit_btn = Button("Exit", [&]() { screen.ExitLoopClosure()(); });

  auto container = Container::Vertical({
      name_input,
      exit_btn,
  });

  auto renderer = Renderer(container, [&] {
    if (!joined) {
      return center(
          vbox({
            vbox(([&]() {
              Elements art_nodes;
              for (const auto& line : ASCII_ART) {
                  art_nodes.push_back(text(std::string(line)));
              }
              return art_nodes;
            })()) | hcenter,

            filler(),

            text("Enter your username and press ENTER to confirm"),
            name_input->Render() | size(WIDTH, EQUAL, 35) | hcenter,
        }) | flex
      );
    }

    return vbox({
        hbox({filler(), text("Welcome to Chatty!"), filler()}),
        filler(),
        hbox({filler(), exit_btn->Render(), filler()}),
    });
  });

  screen.Loop(renderer);
  return 0;
}

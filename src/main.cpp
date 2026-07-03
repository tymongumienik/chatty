#include "ftxui/component/component.hpp" 
#include "ftxui/component/screen_interactive.hpp"
#include "ftxui/dom/elements.hpp"

int main() {
  using namespace ftxui;

  auto screen = ScreenInteractive::Fullscreen();

  std::string name;
  bool joined = false;

  InputOption input_opts;
  input_opts.on_enter = [&] { joined = true; };
  input_opts.multiline = false;

  auto name_input = Input(&name, "username", input_opts);
  auto exit_btn = Button("Exit", [&]() { screen.ExitLoopClosure()(); });

  auto container = Container::Vertical({
      name_input,
      exit_btn,
  });

  auto renderer = Renderer(container, [&] {
    if (!joined) {
      return center(vbox({
          text("Enter your username and press enter to confirm"),
          name_input->Render(),
      }));
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

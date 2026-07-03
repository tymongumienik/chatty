#include "ftxui/component/component.hpp"
#include "ftxui/dom/elements.hpp"

#include "app.hpp"

ftxui::Component App::MakeWaitingForPeerScreen() {
    using namespace ftxui;

    auto back_btn = Button("Back", [this] { SetStage(AppStage::NameInput); });
    auto skip_btn = Button("Skip", [this] { SetStage(AppStage::Chatting); });

    auto container = Container::Vertical({back_btn, skip_btn});

    return Renderer(container, [this, back_btn, skip_btn] {
        return center(vbox({
            text("Connected as: " + state_.username),
            filler(),
            text("Waiting for a peer to connect..."),
            filler(),
            back_btn->Render(),
            skip_btn->Render(),
        }));
    });
}

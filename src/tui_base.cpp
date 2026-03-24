#include "tui_base.hpp"
#include <ftxui/component/component.hpp>

using namespace ftxui;

void TuiBase::run() {
    ScreenInteractive screen = ScreenInteractive::TerminalOutput();
    screen_ = &screen;
    screen_->TrackMouse(false);

    auto root = prepareRoot();

    // Main blocking loop
    screen.Loop(root);

    screen_ = nullptr;
}

void TuiBase::postCustomEvent() {
    if (screen_) screen_->PostEvent(Event::Custom);
}
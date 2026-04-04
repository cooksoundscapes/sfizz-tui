#include "tui_base.hpp"
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

using namespace ftxui;

void TuiBase::run() {
    screen_.TrackMouse(false);
    auto root = prepareRoot();
    // Main blocking loop
    screen_.Loop(root);
}

void TuiBase::init() {
    screen_.TrackMouse(false);
    auto root = prepareRoot();
    loop_.emplace(&screen_, root);
}

bool TuiBase::runOnce(bool shouldRedraw) {
    if (!loop_ || loop_->HasQuitted()) return false;
    if (shouldRedraw)
        screen_.PostEvent(Event::Custom);
    loop_->RunOnce();
    return true;
}

#include "tui_base.hpp"
#include <ftxui/component/component.hpp>
#include <atomic>
#include <thread>

using namespace ftxui;

void TuiBase::run() {
    ScreenInteractive screen = ScreenInteractive::TerminalOutput();
    screen_ = &screen;
    screen_->TrackMouse(false);

    auto root = prepareRoot();

    std::atomic<bool> refresh_ui_continue{true};
    std::thread refresh_thread([&] {
        while (refresh_ui_continue) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // 10 FPS
            if (screen_) screen_->PostEvent(Event::Custom);
        }
    });

    // Main blocking loop
    screen.Loop(root);

    // Finalize refresh thread
    refresh_ui_continue = false;
    if (refresh_thread.joinable()) {
        refresh_thread.join();
    }
    screen_ = nullptr;

}

void TuiBase::postCustomEvent() {
    if (screen_) screen_->PostEvent(Event::Custom);
}
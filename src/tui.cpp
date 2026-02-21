#include "tui.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include <filesystem>

using namespace ftxui;
namespace fs = std::filesystem;

TuiClient::TuiClient(std::string sfzDirectory)
    : sfzDirectory_(std::move(sfzDirectory)) {}

void TuiClient::requestRescan() {
    pendingCommand_.store(UiCommand::RescanDirectory, std::memory_order_release);
    if (screen_) {
        screen_->PostEvent(Event::Custom);
    }
}

void TuiClient::run() {
    ScreenInteractive screen = ScreenInteractive::TerminalOutput();
    screen_ = &screen;

    // UI component
    auto dropdown = Dropdown(&sfzFiles_, &selectedIndex_);

    auto renderer = Renderer(dropdown, [&] {
        return vbox({
            text("SFZ Browser") | bold,
            separator(),
            text("Directory: " + sfzDirectory_),
            separator(),
            dropdown->Render(),
        }) | border;
    });

    // Root component with event handling
    auto root = CatchEvent(renderer, [&](Event event) {
        if (event == Event::Custom) {
            auto cmd = pendingCommand_.exchange(UiCommand::None,
                                                std::memory_order_acquire);
            handleCommand(cmd);
            return true;
        }
        return false;
    });

    // Initial scan (UI thread)
    scanDirectory();

    screen.Loop(root);

    screen_ = nullptr;
}

void TuiClient::handleCommand(UiCommand cmd) {
    switch (cmd) {
        case UiCommand::RescanDirectory:
            scanDirectory();
            break;
        case UiCommand::None:
        default:
            break;
    }
}

void TuiClient::scanDirectory() {
    sfzFiles_.clear();
    selectedIndex_ = 0;

    if (!fs::exists(sfzDirectory_))
        return;

    for (const auto& entry : fs::directory_iterator(sfzDirectory_)) {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() == ".sfz") {
            sfzFiles_.push_back(entry.path().filename().string());
        }
    }

    if (sfzFiles_.empty()) {
        sfzFiles_.push_back("<no .sfz files>");
    }
}
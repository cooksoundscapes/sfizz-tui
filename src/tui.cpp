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

Component TuiClient::createHeader_() {
    return Renderer([&] {
    // Busca os dados via callbacks
    std::string midiName = getMidiDeviceName ? getMidiDeviceName() : "None";
    float cpu = getCpuLoad ? getCpuLoad() : 0.0f;
    bool jackOk = getJackStatus ? getJackStatus() : false;

    // Formatação do JACK
    auto jack_status = jackOk ? text(" JACK: OK ") | bgcolor(Color::Green) | color(Color::Black)
                              : text(" JACK: ERR ") | bgcolor(Color::Red) | color(Color::White);

    // Formatação da CPU (Muda de cor se estiver fritando)
    Color cpuColor = Color::Green;
    if (cpu > 50.0f) cpuColor = Color::Yellow;
    if (cpu > 80.0f) cpuColor = Color::Red;

    return hbox({
        jack_status,
        separator(),
        text(" MIDI [F2]: " + midiName) | flex,
        separator(),
        text(" Load: ") | color(cpuColor),
        gauge(cpu / 100.0f) | size(WIDTH, EQUAL, 10) | color(cpuColor),
    });
});
}

Component TuiClient::createLoadingModal_() {
    return Renderer([&] {
        frame_++;
        return vbox({
            text("loading...") | hcenter,
            spinner(18, frame_) | hcenter | color(Color::Cyan),
        })
        | size(WIDTH, GREATER_THAN, 30)
        | borderDouble
        | bgcolor(Color::Black);
    });
}

Component TuiClient::createMidiSourcesModal_() {
    MenuOption option;
    option.on_enter = [&] {
        auto index = static_cast<size_t>(selectedSourceIndex_);
        if (index < midiSources_.size()) {
            onMidiSourceSelected(midiSources_[index]);
            showMidiSourcesModal_ = false;
        }
    };
    auto sourcesMenu = Menu(&midiSources_, &selectedSourceIndex_, option);

    return Renderer(sourcesMenu, [=] {
        return vbox({
            text("Select MIDI source or ESC to exit"),
            separator(),
            sourcesMenu->Render()
                | vscroll_indicator
                | frame
                | size(HEIGHT, LESS_THAN, 5)
                | size(WIDTH, GREATER_THAN, 30)
        })
        | bgcolor(Color::Black)
        | borderDouble;
    });
}

Component TuiClient::createSfzFileMenu_() {
    MenuOption option;
    option.on_enter = [&] {
        auto index = static_cast<size_t>(selectedSfzIndex_);
        if (index < sfzFiles_.size()) {
            onSfzSelected(sfzFiles_[index]);
            selectedSfzFile_ = sfzFiles_[index];
        }
    };
    auto filesMenu = Menu(&sfzFiles_, &selectedSfzIndex_, option);
   
    return Renderer(filesMenu, [=] {
        return vbox({
            separator(),
            text(sfzDirectory_ + " : " + selectedSfzFile_),
            separator(),
            filesMenu->Render()
                | vscroll_indicator
                | frame
                | size(HEIGHT, LESS_THAN, sfzFileMenuLineCount_)
        });
    });
}

void TuiClient::run() {
    ScreenInteractive screen = ScreenInteractive::TerminalOutput();
    screen_ = &screen;
    screen_->TrackMouse(false);

    auto header = createHeader_();
    auto sfzMenu = createSfzFileMenu_();
    auto midiSourcesModal = createMidiSourcesModal_();
    auto loadingModal = createLoadingModal_();

    auto page = Container::Vertical({header, sfzMenu}) | border;

    auto overlay1 = Modal(page, midiSourcesModal, &showMidiSourcesModal_);
    auto overlay2 = Modal(overlay1, loadingModal, &engineIsLoading_);

    auto root = CatchEvent(overlay2, [&](Event event) {
        if (event == Event::Custom) {
            auto cmd = pendingCommand_.exchange(
                UiCommand::None,
                std::memory_order_acquire
            );
            handleCommand(cmd);
            return true;
        }
        if (event == Event::F2) {
            midiSources_ = getMidiDevices();
            showMidiSourcesModal_ = true;
            return true; // Consome o evento para ninguém mais usar F2
        }
        if (event == Event::Escape && showMidiSourcesModal_) {
            showMidiSourcesModal_ = false;
            return true;
        }
        return false;
    });

    // Initial scan (UI thread)
    scanDirectory();

    // thread for keeping the screen live
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

void TuiClient::postCustomEvent() {
    if (screen_) screen_->PostEvent(Event::Custom);
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
    selectedSfzIndex_ = 0;

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
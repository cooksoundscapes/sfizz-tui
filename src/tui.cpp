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

Component TuiClient::getHeader_() {
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
        text(" MIDI: " + midiName) | flex,
        separator(),
        text(" Load: ") | color(cpuColor),
        gauge(cpu / 100.0f) | size(WIDTH, EQUAL, 10) | color(cpuColor),
    });
});
}

void TuiClient::run() {
    ScreenInteractive screen = ScreenInteractive::TerminalOutput();
    screen_ = &screen;

    MenuOption option;
    option.on_change = [&] {
        auto index = static_cast<size_t>(selectedIndex_);
        if (index < sfzFiles_.size()) {
            onSfzSelected(sfzFiles_[index]);
        }
    };
    auto dropdown = Menu(&sfzFiles_, &selectedIndex_, option);

    auto header = getHeader_();

    auto loading_modal = Renderer([&] {
        frame_++;
        return vbox({
            text("loading...") | hcenter,
            spinner(18, frame_) | hcenter | color(Color::Cyan),
        }) | size(WIDTH, GREATER_THAN, 30) | borderDouble | bgcolor(Color::Black);
    });
    bool loading = isEngineLoading && isEngineLoading();

    auto content = Renderer(dropdown, [&] {
        return vbox({
            separator(),
            text("Directory: " + sfzDirectory_),
            separator(),
            dropdown->Render(),
        });
    });

    auto page = Container::Vertical({header, content}) | border;
    
    auto composition = Modal(page, loading_modal, &loading);

    // Root component with event handling
    auto root = CatchEvent(composition, [&](Event event) {
        if (event == Event::Custom) {
            auto cmd = pendingCommand_.exchange(
                UiCommand::None,
                std::memory_order_acquire
            );
            handleCommand(cmd);
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

    screen.Loop(root);

    // Finaliza a thread de refresh
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
#include "tui.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>

#include <filesystem>

#include <iostream>

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
    bool engineOk = getEngineStatus ? getEngineStatus() : false;

    // Formatação do JACK
    auto engine_status = engineOk ? text(" Engine: OK ") | bgcolor(Color::Green) | color(Color::Black)
                              : text(" Engine: ERR ") | bgcolor(Color::Red) | color(Color::White);

    // Formatação da CPU (Muda de cor se estiver fritando)
    Color cpuColor = Color::Green;
    if (cpu > 50.0f) cpuColor = Color::Yellow;
    if (cpu > 80.0f) cpuColor = Color::Red;

    return hbox({
        engine_status,
        separator(),
        text(" MIDI [F2]: " + midiName) | flex,
        separator(),
        text(" Load: ") | color(cpuColor),
        gauge(cpu / 100.0f) | size(WIDTH, EQUAL, 10) | color(cpuColor),
    });
});
}

Component TuiClient::createLoggerView_() {
    return Renderer([&] {
        auto logs = getLogBuffer();

        return vbox({
            window(text(" [F3] Engine Logs ") | hcenter | bold,
                paragraph(std::string(logs)) 
                | vscroll_indicator 
                | yframe // Permite scroll se o texto exceder a altura
                | size(HEIGHT, LESS_THAN, 20) // Limita a altura do overlay
            )
        }) 
        | bgcolor(Color::Black) // Fundo opaco para não misturar com a UI atrás
        | center;               // Centraliza o overlay na tela
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
            onSfzSelected(sfzFiles_[index].filePath);
            selectedSfzFile_ = sfzFiles_[index].displayName;
        }
    };
    auto filesMenu = Menu(&fileNames_, &selectedSfzIndex_, option);
   
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
    auto loggerModal = createLoggerView_();

    auto page = Container::Vertical({header, sfzMenu}) | border;

    auto overlay1 = Modal(page, midiSourcesModal, &showMidiSourcesModal_);
    auto overlay2 = Modal(overlay1, loggerModal, &showLogs_);
    auto overlay_final = Modal(overlay2, loadingModal, &engineIsLoading_);

    auto root = CatchEvent(overlay_final, [&](Event event) {
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
            showLogs_ = false;
            return true; // Consome o evento para ninguém mais usar F2
        }
        if (event == Event::F3) {
            showLogs_ = !showLogs_;
            showMidiSourcesModal_ = false;
            return true;
        }
        if (event == Event::Escape) {
            showMidiSourcesModal_ = false;
            showLogs_ = false;
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

    for (const auto& entry : fs::recursive_directory_iterator(sfzDirectory_)) {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() == ".sfz") {
            sfzFiles_.push_back({
                .displayName = entry.path().filename().string(),
                .filePath = entry.path().string()
            });
        }
    }

    if (sfzFiles_.empty()) {
        sfzFiles_.push_back({
            .displayName = "<no .sfz files>",
            .filePath = ""
        });
        return;
    }
    std::sort(sfzFiles_.begin(), sfzFiles_.end(),
        [](const SfzFile& a, const SfzFile& b) { return a.displayName < b.displayName; }
    );
    fileNames_.reserve(sfzFiles_.size());
    for (const auto& file : sfzFiles_) {
        fileNames_.push_back(file.displayName);
    }
}
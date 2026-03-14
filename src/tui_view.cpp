#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include "tui_view.hpp"


using namespace ftxui;

void TuiView::createHeader_() {
    header = Renderer([&] {
    std::string midiName = app.getMidiDeviceName();
    std::string midiLabel = midiName.empty() ? "F2 Select MIDI" : midiName;
    float cpu = app.getCpuLoad();
    bool engineOk = app.getEngineStatus();

    auto engine_status = engineOk ? text(" Engine: OK ") | bgcolor(Color::Green) | color(Color::Black)
                              : text(" Engine: ERR ") | bgcolor(Color::Red) | color(Color::White);

    Color cpuColor = Color::Green;
    if (cpu > 50.0f) cpuColor = Color::Yellow;
    if (cpu > 80.0f) cpuColor = Color::Red;

    return hbox({
        engine_status,
        separator(),
        text(midiLabel) | flex,
        separator(),
        text(" Load: ") | color(cpuColor),
        gauge(cpu / 100.0f) | size(WIDTH, EQUAL, 10) | color(cpuColor),
    });
});
}

void TuiView::createLoggerView_() {
    loggerModal = Renderer([&] {
        auto logs = app.getLogBuffer();

        return vbox({
            window(text(" [F3] Engine Logs ") | hcenter | bold,
                paragraph(std::string(logs)) 
                | vscroll_indicator 
                | yframe
                | size(HEIGHT, LESS_THAN, 20)
            )
        }) 
        | bgcolor(Color::Black)
        | center;
    });
}

void TuiView::createLoadingModal_() {
    loadingModal = Renderer([&] {
        provider.advanceFrame();
        return vbox({
            text("loading...") | hcenter,
            spinner(18, provider.getFrame()) | hcenter | color(Color::Cyan),
        })
        | size(WIDTH, GREATER_THAN, 30)
        | borderDouble
        | bgcolor(Color::Black);
    });
}

void TuiView::createMidiSourcesModal_() {
    MenuOption option;
    option.on_enter = [&] {
        auto index = static_cast<size_t>(selectedSourceIndex_);
        if (index < midiSources_.size()) {
            app.onMidiSourceSelected(midiSources_[index]);
            showMidiSourcesModal_ = false;
        }
    };
    auto sourcesMenu = Menu(&midiSources_, &selectedSourceIndex_, option);

    midiSourcesModal = Renderer(sourcesMenu, [=] {
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

void TuiView::createSfzFileMenu_() {
    MenuOption option;
    option.on_enter = [&] {
        selectedSfzFile_ = provider.onSfzSelected(selectedSfzIndex_);
    };
    filesMenu = Menu(provider.getFilesList(), &selectedSfzIndex_, option);

    tagsContainer = Container::Vertical({});
    for (auto& tag : provider.getTags()) {
        auto opt = CheckboxOption::Simple();
        opt.on_change = [this] { 
            provider.updateFilteredList(); 
        };
        tagsContainer->Add(Checkbox(&tag.name, &tag.enabled, opt));
    }

    auto splitLayout = Container::Horizontal({
        filesMenu,
        tagsContainer
    });
   
    sfzMenu = Renderer(splitLayout, [=, this] {
        auto keySwitch = app.getActiveKeyswitch();
        auto dim = Terminal::Size();
        return vbox({
            separator(),
            text(provider.getSfzDir() + " : " + selectedSfzFile_ + " (" + keySwitch + ")"),
            separator(),
            hbox({
                filesMenu->Render()
                    | vscroll_indicator
                    | frame
                    | size(HEIGHT, LESS_THAN, sfzFileMenuLineCount_)
                    | size(WIDTH, EQUAL, static_cast<int>(dim.dimx * 0.7f)),
                separator(),
                tagsContainer->Render()
                    | vscroll_indicator
                    | frame
                    | size(HEIGHT, LESS_THAN, provider.getTags().size())
            })
        });
    });
}

Component TuiView::root() {
    createHeader_();
    createSfzFileMenu_();
    createMidiSourcesModal_();
    createLoadingModal_();
    createLoggerView_();

    page = Container::Vertical({header, sfzMenu}) | border;

    overlay1 = Modal(page, midiSourcesModal, &showMidiSourcesModal_);
    overlay2 = Modal(overlay1, loggerModal, &showLogs_);
    overlay_final = Modal(overlay2, loadingModal, &engineIsLoading_);

    return CatchEvent(overlay_final, [&](Event event) {
        if (event == Event::F2) {
            midiSources_ = app.getMidiDevices();
            showMidiSourcesModal_ = true;
            showLogs_ = false;
            return true;
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
}
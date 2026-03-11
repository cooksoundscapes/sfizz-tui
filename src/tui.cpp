#include "tui.hpp"
#include "sfz_cache.hpp"
#include "sfz_parser.hpp"
#include "ui_commands.hpp"

#include <ftxui/component/component.hpp>
#include <ftxui/component/component_base.hpp>
#include <ftxui/dom/elements.hpp>
#include <filesystem>
#include <memory>

using namespace ftxui;
namespace fs = std::filesystem;

TuiClient::TuiClient(std::string sfzDirectory)
    : sfzDirectory_(std::move(sfzDirectory)) {
        midiParser = std::make_unique<MidiParser>();
    }


Component TuiClient::createHeader_() {
    return Renderer([&] {
    std::string midiName = getMidiDeviceName ? getMidiDeviceName() : "";
    std::string midiLabel = midiName.empty() ? "F2 Select MIDI" : midiName;
    float cpu = getCpuLoad ? getCpuLoad() : 0.0f;
    bool engineOk = getEngineStatus ? getEngineStatus() : false;

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

Component TuiClient::createLoggerView_() {
    return Renderer([&] {
        auto logs = getLogBuffer();

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
        if (index < filteredIndices_.size()) {
            int realIndex = filteredIndices_[index];
            onSfzSelected(sfzFiles_[realIndex].filePath);
            selectedSfzFile_ = sfzFiles_[realIndex].displayName;
        }
    };
    auto filesMenu = Menu(&fileNames_, &selectedSfzIndex_, option);

    auto tagsContainer = Container::Vertical({});
    for (auto& tag : availableTags_) {
        auto opt = CheckboxOption::Simple();
        opt.on_change = [this] { 
            updateFilteredList_(); 
        };
        tagsContainer->Add(Checkbox(&tag.name, &tag.enabled, opt));
    }

    auto splitLayout = Container::Horizontal({
        filesMenu,
        tagsContainer
    });
   
    return Renderer(splitLayout, [=, this] {
        auto keySwitch = getActiveKeyswitch ? getActiveKeyswitch() : "";
        auto dim = Terminal::Size();
        return vbox({
            separator(),
            text(sfzDirectory_ + " : " + selectedSfzFile_ + " (" + keySwitch + ")"),
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
                    | size(HEIGHT, LESS_THAN, availableTags_.size())
            })
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
        if (event == Event::F2) {
            midiSources_ = getMidiDevices();
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

    scanDirectory();

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

void TuiClient::scanDirectory() {
    sfzFiles_.clear();
    selectedSfzIndex_ = 0;

    if (!fs::exists(sfzDirectory_))
        return;

    for (const auto& entry : fs::recursive_directory_iterator(sfzDirectory_)) {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() == ".sfz") {
            SfzMetaData metadata = SfzParser::load(entry.path().string());
            std::string displayName = entry.path().filename().string();

            sfzFiles_.push_back({
                .displayName = displayName,
                .filePath = entry.path().string(),
                .tagMask = metadata.tag_mask
            });
        }
    }

    if (sfzFiles_.empty()) {
        sfzFiles_.push_back({
            .displayName = "<no .sfz files>",
            .filePath = "",
            .tagMask = 0
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

void TuiClient::updateFilteredList_() {
    fileNames_.clear();
    filteredIndices_.clear();

    fileNames_.reserve(sfzFiles_.size());
    filteredIndices_.reserve(sfzFiles_.size());

    uint64_t activeMask = 0;
    for (const auto& t : availableTags_) {
        if (t.enabled) activeMask |= t.bit;
    }

    for (int i = 0; i < (int)sfzFiles_.size(); ++i) {
        if (activeMask == 0 || (sfzFiles_[i].tagMask & activeMask)) {
            std::string label = sfzFiles_[i].displayName;
            if (sfzFiles_[i].tagMask & Tag::FAVORITE) label = "★ " + label;
            
            fileNames_.push_back(label);
            filteredIndices_.push_back(i);
        }
    }
}

void TuiClient::loadMidiMap(std::string filepath) {
    if (!midiParser->loadMap(filepath)) {
        std::cerr << "Error loading MIDI map file " << filepath << std::endl;
    }
}

void TuiClient::handleMidiEvent(MidiEvent e) {
    if (!midiParser->isLoaded()) return;

    UiCommand cmd = midiParser->parseEvent(e.type, e.channel, e.data1);

    bool isDown =
        (e.type == MidiInputType::NOTE_ON && e.data2 > 0) || 
        (e.type == MidiInputType::CONTROL_CHANGE);

    if (cmd != UiCommand::NONE && isDown) {
        switch(cmd) {
            case UiCommand::NAV_DOWN:       screen_->PostEvent(Event::ArrowDown); break;
            case UiCommand::NAV_UP:         screen_->PostEvent(Event::ArrowUp); break;
            case UiCommand::NAV_LEFT:       screen_->PostEvent(Event::ArrowLeft); break;
            case UiCommand::NAV_RIGHT:      screen_->PostEvent(Event::ArrowRight); break;
            case UiCommand::PAGE_UP:        screen_->PostEvent(Event::PageUp); break;
            case UiCommand::PAGE_DOWN:      screen_->PostEvent(Event::PageDown); break;
            case UiCommand::BROWSE_MENU: {
                int delta = (int)e.data2 - 64;
                if (delta > 0) {
                    for (int i = 0; i < std::abs(delta); ++i) {
                        screen_->PostEvent(Event::ArrowDown);
                    }
                } 
                else if (delta < 0) {
                    for (int i = 0; i < std::abs(delta); ++i) {
                        screen_->PostEvent(Event::ArrowUp);
                    }
                }
                screen_->PostEvent(Event::Return);
                break;
            }
            case UiCommand::OPEN_MIDI_MENU: screen_->PostEvent(Event::F2); break;
            case UiCommand::OPEN_LOGS:      screen_->PostEvent(Event::F3); break;
            case UiCommand::CONFIRM_SELECT: screen_->PostEvent(Event::Return); break;
            case UiCommand::BACK_CANCEL:    screen_->PostEvent(Event::Escape); break;
            case UiCommand::FOCUS_FILES:    screen_->PostEvent(Event::Special("focus_files")); break;
            case UiCommand::FOCUS_TAGS:     screen_->PostEvent(Event::Special("focus_tags")); break;
            case UiCommand::TAG_TOGGLE_0:   screen_->PostEvent(Event::Special("tag_0")); break;
            case UiCommand::TAG_TOGGLE_1:   screen_->PostEvent(Event::Special("tag_1")); break;
            case UiCommand::TAG_TOGGLE_2:   screen_->PostEvent(Event::Special("tag_2")); break;
            case UiCommand::TAG_TOGGLE_3:   screen_->PostEvent(Event::Special("tag_3")); break;
            case UiCommand::TAG_CLEAR_ALL:  screen_->PostEvent(Event::Special("tag_clear")); break;
            case UiCommand::ENGINE_RELOAD:  screen_->PostEvent(Event::Special("reload")); break;
            default: break;
        }
    }
}
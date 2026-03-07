#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <string>
#include <vector>
#include <string_view>
#include <memory>

#include "midi_parser.hpp"
#include "sfz_cache.hpp"

class TuiClient {
public:
    explicit TuiClient(std::string sfzDirectory);

    void run();

    std::function<void(const std::string&)> onSfzSelected;
    std::function<bool()> isEngineLoading;
    std::function<std::string()> getMidiDeviceName;
    std::function<float()> getCpuLoad; // 0.0 a 100.0
    std::function<bool()> getEngineStatus;
    std::function<std::vector<std::string>()> getMidiDevices;
    std::function<void(const std::string&)> onMidiSourceSelected;
    std::function<std::string_view()> getLogBuffer;
    std::function<std::string()> getActiveKeyswitch;

    void postCustomEvent();

    void clear() { if (screen_) screen_->Clear(); }

    void loadMidiMap(std::string filepath);
    void handleMidiEvent(MidiEvent);

private:
    std::unique_ptr<MidiParser> midiParser;

    void scanDirectory();
    void updateFilteredList_();

    // Components
    ftxui::Component createHeader_();
    ftxui::Component createLoadingModal_();
    ftxui::Component createMidiSourcesModal_();
    ftxui::Component createSfzFileMenu_();
    ftxui::Component createLoggerView_();

    // flags
    bool engineIsLoading_ = false;
    bool showMidiSourcesModal_ = false;
    bool showLogs_ = false;

    struct TagFilter {
        std::string name;
        uint64_t bit;
        bool enabled = false;
    };
    std::vector<TagFilter> availableTags_ = {
        {"FAVORITE", Tag::FAVORITE},
        {"STRINGS", Tag::STRINGS},
        {"MELLOTRON", Tag::MELLOTRON},
        {"WOODWIND", Tag::WOODWIND},
        {"BRASS", Tag::BRASS},
        {"KEYS", Tag::KEYS},
        {"TINES", Tag::TINES},
        {"PERC", Tag::PERC},
        {"PORTISHEAD", Tag::PORTISHEAD},
    };

    std::string sfzDirectory_;
    const int sfzFileMenuLineCount_ = 10;

    int selectedSfzIndex_ = 0;
    int selectedSourceIndex_ = 0;

    std::string selectedSfzFile_ = "None";

    struct SfzFile {
        std::string displayName;
        std::string filePath;
        uint64_t tagMask;
    };
    std::vector<SfzFile> sfzFiles_;
    std::vector<std::string> fileNames_;
    std::vector<std::string> midiSources_;
    std::vector<int> filteredIndices_;

    ftxui::ScreenInteractive* screen_ = nullptr;
    int frame_ = 0;
};
#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <atomic>
#include <string>
#include <vector>
#include <string_view>

class TuiClient {
public:
    explicit TuiClient(std::string sfzDirectory);

    // thread-safe
    void requestRescan();

    // bloqueia até sair da UI
    void run();

    std::function<void(const std::string&)> onSfzSelected;
    std::function<bool()> isEngineLoading;
    std::function<std::string()> getMidiDeviceName;
    std::function<float()> getCpuLoad; // 0.0 a 100.0
    std::function<bool()> getEngineStatus;
    std::function<std::vector<std::string>()> getMidiDevices;
    std::function<void(const std::string&)> onMidiSourceSelected;
    std::function<std::string_view()> getLogBuffer;

    void postCustomEvent();

    void clear() { if (screen_) screen_->Clear(); }

private:
    enum class UiCommand {
        None,
        RescanDirectory,
    };

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

    void handleCommand(UiCommand cmd);
    void scanDirectory();

    std::string sfzDirectory_;
    const int sfzFileMenuLineCount_ = 10;

    int selectedSfzIndex_ = 0;
    int selectedSourceIndex_ = 0;

    std::string selectedSfzFile_ = "None";

    struct SfzFile {
        std::string displayName;
        std::string filePath;
    };
    std::vector<SfzFile> sfzFiles_;
    std::vector<std::string> fileNames_;
    std::vector<std::string> midiSources_;

    std::atomic<UiCommand> pendingCommand_{UiCommand::None};

    ftxui::ScreenInteractive* screen_ = nullptr;
    int frame_ = 0;
};
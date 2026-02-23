#pragma once

#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

#include <atomic>
#include <string>
#include <vector>

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
    std::function<bool()> getJackStatus;

    void postCustomEvent();

private:
    enum class UiCommand {
        None,
        RescanDirectory,
    };

    ftxui::Component getHeader_();

    void handleCommand(UiCommand cmd);
    void scanDirectory();

    std::string sfzDirectory_;

    std::vector<std::string> sfzFiles_;
    int selectedIndex_ = 0;

    std::atomic<UiCommand> pendingCommand_{UiCommand::None};

    ftxui::ScreenInteractive* screen_ = nullptr;
    int frame_ = 0;
};
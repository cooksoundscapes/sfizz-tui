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

private:
    enum class UiCommand {
        None,
        RescanDirectory,
    };

    void handleCommand(UiCommand cmd);
    void scanDirectory();

private:
    std::string sfzDirectory_;

    std::vector<std::string> sfzFiles_;
    int selectedIndex_ = 0;

    std::atomic<UiCommand> pendingCommand_{UiCommand::None};

    ftxui::ScreenInteractive* screen_ = nullptr;
};
#pragma once
#include <ftxui/component/screen_interactive.hpp>
#include "ftxui/component/loop.hpp"
#include <optional>


class TuiBase {
public:
    virtual ~TuiBase() = default;

    TuiBase() : screen_(ftxui::ScreenInteractive::TerminalOutput()) {}

    void run();

    bool loopHasQuitted() { if (loop_) return loop_->HasQuitted(); return true; }

    void clear() { screen_.Clear(); }

    bool runOnce(bool);

protected:
    ftxui::ScreenInteractive screen_;
    std::optional<ftxui::Loop> loop_;

    void init();

    virtual ftxui::Component prepareRoot() = 0;
};
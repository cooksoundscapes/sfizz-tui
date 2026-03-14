#pragma once
#include <ftxui/component/screen_interactive.hpp>

class TuiBase {
public:
    virtual ~TuiBase() = default;

    void run();

    void clear() { if (screen_) screen_->Clear(); }

    void postCustomEvent();

protected:
    ftxui::ScreenInteractive* screen_ = nullptr;
    int frame_ = 0;

    virtual ftxui::Component prepareRoot() = 0;
};
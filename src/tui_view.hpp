#pragma once
#include <ftxui/component/component.hpp>
#include <vector>

#include <sfz_cache.hpp>

template<typename T>
using Action = std::function<void(T)>;
template<typename T>
using Getter = std::function<T()>;
template<typename I, typename O>
using Resolver = std::function<O(I)>;
using Trigger = std::function<void()>;

using el = ftxui::Component;

class TuiView {
public:
    struct AppContext {
        Getter<bool> isEngineLoading;
        Getter<bool> getEngineStatus;
        Getter<std::string> getMidiDeviceName;
        Getter<float> getCpuLoad;
        Getter<std::vector<std::string>> getMidiDevices;
        Getter<std::string_view> getLogBuffer;
        Getter<std::string> getActiveKeyswitch;
        Action<const std::string&> onMidiSourceSelected;
    };

    struct UiContext {
        // internally defined by core
        Trigger advanceFrame;
        Trigger updateFilteredList;
        Getter<const int&> getFrame;
        Getter<std::string> getSfzDir;
        Getter<std::vector<TagFilter>&> getTags;
        Getter<const std::vector<std::string>&> getFilesList;
        Resolver<int, std::string> onSfzSelected;
    };

    TuiView(AppContext app, UiContext ui)
    : app(std::move(app)), provider(std::move(ui)) {}

    ftxui::Component root();

    void resetSfzIndex() { selectedSfzIndex_ = 0; }

private:
    void createHeader_();
    void createLoadingModal_();
    void createMidiSourcesModal_();
    void createSfzFileMenu_();
    void createLoggerView_();

    el header;
    el sfzMenu;
    el midiSourcesModal;
    el loadingModal;
    el loggerModal;

    el page;
    el overlay1;
    el overlay2;
    el overlay_final;
    el tagsContainer;
    el filesMenu;

    AppContext app;
    UiContext provider;

    bool engineIsLoading_ = false;
    bool showMidiSourcesModal_ = false;
    bool showLogs_ = false;

    int selectedSfzIndex_ = 0;
    int selectedSourceIndex_ = 0;

    std::string selectedSfzFile_ = "None";
    const int sfzFileMenuLineCount_ = 10;

    std::vector<std::string> midiSources_;
};
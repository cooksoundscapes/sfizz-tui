#include "tui_base.hpp"
#include "tui_view.hpp"
#include "midi_parser.hpp"
#include "sfz_cache.hpp"

#include <memory>
#include <vector>
#include <functional>

class TuiClient : public TuiBase {
public:
    explicit TuiClient(std::string sfzDirectory);

    void loadMidiMap(std::string filepath);
    void handleMidiEvent(MidiEvent);

    std::function<void(const std::string&)> onSfzSelected;

    void start();
    TuiView::AppContext& context() { return app; }

private:
    ftxui::Component prepareRoot() override { return view->root(); }

    std::unique_ptr<MidiParser> midiParser;
    std::unique_ptr<TuiView> view;

    TuiView::AppContext app;
    TuiView::UiContext provider;

    void scanDirectory();
    void updateFilteredList_();
    std::string loadSfz_(int index);

    struct SfzFile {
        std::string displayName;
        std::string filePath;
        uint64_t tagMask;
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
    std::vector<SfzFile> sfzFiles_;
    std::vector<std::string> fileNames_;
    std::vector<int> filteredIndices_;

    std::string sfzDirectory_;
};
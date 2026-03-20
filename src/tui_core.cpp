#include <cassert>
#include <ftxui/component/component.hpp>
#include <filesystem>

#include "tui_core.hpp"
#include "sfz_parser.hpp"

using namespace ftxui;
namespace fs = std::filesystem;


TuiClient::TuiClient(std::string sfzDirectory)
    : sfzDirectory_(std::move(sfzDirectory))
{
    midiParser = std::make_unique<MidiParser>();
}

void TuiClient::run() {
    provider = {
        .advanceFrame = [this](){
            this->frame_++;
        },
        .updateFilteredList = [this](){
            this->updateFilteredList_();
        },
        .getFrame = [this]() -> const int& {
            return this->frame_;
        },
        .getSfzDir = [this](){
            return this->sfzDirectory_;
        },
        .getTags = [this]() -> std::vector<TagFilter>& {
            return this->availableTags_;
        },
        .getFilesList = [this]() -> std::vector<std::string>& {
            return this->fileNames_;
        },
        .onSfzSelected = [this](int i){
            return this->loadSfz_(i);
        }
    };

    view = std::make_unique<TuiView>(std::move(app), std::move(provider));
    
    scanDirectory();

    TuiBase::run();
}

std::string TuiClient::loadSfz_(int idx) {
   auto index = static_cast<size_t>(idx);
    if (index < filteredIndices_.size()) {
        int realIndex = filteredIndices_[index];
        onSfzSelected(sfzFiles_[realIndex].filePath);
        return sfzFiles_[realIndex].displayName;
    }
    return "None";
}

void TuiClient::scanDirectory() {
    if (view) {
        view->resetSfzIndex();
    }

    sfzFiles_.clear();

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
    updateFilteredList_();
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

    std::cerr << "---------------------\n";

    for (int i = 0; i < (int)sfzFiles_.size(); ++i) {
        if (activeMask == 0 || (sfzFiles_[i].tagMask & activeMask)) {
            std::string label = sfzFiles_[i].displayName;
            if (sfzFiles_[i].tagMask & Tag::FAVORITE) label = "★ " + label;
            
            fileNames_.push_back(label);
            filteredIndices_.push_back(i);
            std::cerr << "found " << label << '\n';
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
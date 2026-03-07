#include <cstdint>
#include <string>
#include <tuple>
#include <map>

#include "ui_commands.hpp"
#include <core/midi_types.hpp>

class MidiParser {
public:
    struct MidiBinding {
        MidiInputType type;
        uint8_t channel;
        uint8_t id;

        // enables MidiBinding to be a map key (custom comparison operator)
        bool operator<(const MidiBinding& other) const {
            return std::tie(type, channel, id) < std::tie(other.type, other.channel, other.id);
        }
    };

    bool loadMap(const std::string& filePath);
    bool isLoaded() { return hasMapLoaded_; }

    UiCommand parseEvent(MidiInputType type, uint8_t channel, uint8_t id) const;

private:
    bool hasMapLoaded_ = false;
    std::map<MidiBinding, UiCommand> bindings;
};
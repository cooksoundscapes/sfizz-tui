#include "midi_parser.hpp"
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>

static const std::unordered_map<std::string, MidiInputType> StringToMidiType = {
    {"NOTE", MidiInputType::NOTE_ON},
    {"CC",   MidiInputType::CONTROL_CHANGE},
    {"PC",   MidiInputType::PROGRAM_CHANGE}
};

static const std::unordered_map<std::string, UiCommand> StringToUiCommand = {
#define X(name) {#name, UiCommand::name},
    UI_COMMANDS_LIST
#undef X
};


bool MidiParser::loadMap(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) return false;

    bindings.clear();
    std::string line;
    bool atMappings = false;

    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue;

        if (line.find("[mappings]") != std::string::npos) {
            atMappings = true;
            continue;
        }

        if (!atMappings) continue;

        std::stringstream ss(line);
        std::string typeStr, cmdStr;
        int channel, id;

        // Example: NOTE 1 36 CONFIRM_SELECT
        if (!(ss >> typeStr >> channel >> id >> cmdStr)) continue;

        if (StringToMidiType.count(typeStr) && StringToUiCommand.count(cmdStr)) {
            MidiBinding binding;
            binding.type = StringToMidiType.at(typeStr);
            binding.channel = static_cast<uint8_t>(channel);
            binding.id = static_cast<uint8_t>(id);

            bindings[binding] = StringToUiCommand.at(cmdStr);
        }
    }

    std::cerr << "MIDI map loaded successfully;\n";

    hasMapLoaded_ = true;
    return true;
}

UiCommand MidiParser::parseEvent(MidiInputType type, uint8_t channel, uint8_t id) const {
    MidiBinding query{ type, channel, id };

    auto it = bindings.find(query);
    if (it != bindings.end()) {
        return it->second;
    }

    return UiCommand::NONE;
}
#pragma once
#include <cstdint>
#include <string>

enum Tag: uint64_t {
    NONE = 0,
    FAVORITE = 1 << 0,
    STRINGS = 1 << 1,
    MELLOTRON = 1 << 2,
    WOODWIND = 1 << 3,
    BRASS = 1 << 4,
    KEYS = 1 << 5,
    TINES = 1 << 6,
    PERC = 1 << 7,
    PORTISHEAD = 1 << 8,
};

struct TagFilter {
    std::string name;
    uint64_t bit;
    bool enabled = false;
};

struct SwitchEntry {
    bool active = false;
    char label[32] = {0};
};

struct SfzMetaData {
    uint64_t original_mtime;
    int32_t default_switch = -1;
    uint64_t tag_mask;
    SwitchEntry keyswitch_map[128];
};

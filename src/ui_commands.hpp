#pragma once 

#define UI_COMMANDS_LIST \
    X(NAV_UP) \
    X(NAV_DOWN) \
    X(PAGE_UP) \
    X(PAGE_DOWN) \
    X(BROWSE_MENU) \
    X(NAV_LEFT) \
    X(NAV_RIGHT) \
    X(CONFIRM_SELECT) \
    X(BACK_CANCEL) \
    X(FOCUS_FILES) \
    X(FOCUS_TAGS) \
    X(OPEN_MIDI_MENU) \
    X(OPEN_LOGS) \
    X(TAG_TOGGLE_0) \
    X(TAG_TOGGLE_1) \
    X(TAG_TOGGLE_2) \
    X(TAG_TOGGLE_3) \
    X(TAG_CLEAR_ALL) \
    X(ENGINE_RELOAD)

enum class UiCommand {
    NONE = 0,
#define X(name) name,
    UI_COMMANDS_LIST
#undef X
};
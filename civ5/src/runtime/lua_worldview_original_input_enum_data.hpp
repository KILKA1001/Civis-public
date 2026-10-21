#pragma once
#include <cstdint>

namespace civ5::runtime::detail {
// Private proposal data only. InterfaceModeTypes follows the pinned SDK Lua registration.
// Input messages and required VK constants follow authentic EXE integer registrations.
// These values grant no issuer, context, lifetime, or input-dispatch authority.
// Graph Mouse.eLClick/eRClick belongs to a separate table and is not replaced here.
struct BnwWorldViewOriginalInputEnumEntry {
    const char* table;
    const char* name;
    std::int64_t value;
};
inline constexpr BnwWorldViewOriginalInputEnumEntry bnw_worldview_original_input_enum_entries[] {
    {"InterfaceModeTypes", "NO_INTERFACEMODE", -1},
    {"InterfaceModeTypes", "INTERFACEMODE_DEBUG", 0},
    {"InterfaceModeTypes", "INTERFACEMODE_SELECTION", 1},
    {"InterfaceModeTypes", "INTERFACEMODE_PING", 2},
    {"InterfaceModeTypes", "INTERFACEMODE_MOVE_TO", 3},
    {"InterfaceModeTypes", "INTERFACEMODE_MOVE_TO_TYPE", 4},
    {"InterfaceModeTypes", "INTERFACEMODE_MOVE_TO_ALL", 5},
    {"InterfaceModeTypes", "INTERFACEMODE_ROUTE_TO", 6},
    {"InterfaceModeTypes", "INTERFACEMODE_AIRLIFT", 7},
    {"InterfaceModeTypes", "INTERFACEMODE_NUKE", 8},
    {"InterfaceModeTypes", "INTERFACEMODE_PARADROP", 9},
    {"InterfaceModeTypes", "INTERFACEMODE_ATTACK", 10},
    {"InterfaceModeTypes", "INTERFACEMODE_RANGE_ATTACK", 11},
    {"InterfaceModeTypes", "INTERFACEMODE_AIRSTRIKE", 12},
    {"InterfaceModeTypes", "INTERFACEMODE_AIR_SWEEP", 13},
    {"InterfaceModeTypes", "INTERFACEMODE_REBASE", 14},
    {"InterfaceModeTypes", "INTERFACEMODE_PLACE_UNIT", 15},
    {"InterfaceModeTypes", "INTERFACEMODE_EMBARK", 16},
    {"InterfaceModeTypes", "INTERFACEMODE_DISEMBARK", 17},
    {"InterfaceModeTypes", "INTERFACEMODE_GIFT_UNIT", 18},
    {"InterfaceModeTypes", "INTERFACEMODE_CITY_PLOT_SELECTION", 19},
    {"InterfaceModeTypes", "INTERFACEMODE_PURCHASE_PLOT", 20},
    {"InterfaceModeTypes", "INTERFACEMODE_CITY_RANGE_ATTACK", 21},
    {"InterfaceModeTypes", "INTERFACEMODE_GIFT_TILE_IMPROVEMENT", 22},
    {"InterfaceModeTypes", "INTERFACEMODE_ESTABLISH_TRADE_ROUTE", 23},
    {"InterfaceModeTypes", "NUM_INTERFACEMODE_TYPES", 24},
    {"KeyEvents", "WM_KEYDOWN", 256},
    {"KeyEvents", "KeyDown", 256},
    {"KeyEvents", "WM_KEYUP", 257},
    {"KeyEvents", "KeyUp", 257},
    {"MouseEvents", "MouseFirst", 512},
    {"MouseEvents", "MouseWheel", 522},
    {"MouseEvents", "MouseMove", 512},
    {"MouseEvents", "LButtonDown", 513},
    {"MouseEvents", "LButtonUp", 514},
    {"MouseEvents", "LButtonDoubleClick", 515},
    {"MouseEvents", "RButtonDown", 516},
    {"MouseEvents", "RButtonUp", 517},
    {"MouseEvents", "RButtonDoubleClick", 518},
    {"MouseEvents", "MButtonDown", 519},
    {"MouseEvents", "MButtonUp", 520},
    {"MouseEvents", "MButtonDoubleClick", 521},
    {"MouseEvents", "XButtonDown", 523},
    {"MouseEvents", "XButtonUp", 524},
    {"MouseEvents", "XButtonDoubleClick", 525},
    {"MouseEvents", "MouseLast", 526},
    {"MouseEvents", "PointerUp", 583},
    {"MouseEvents", "PointerDown", 582},
    {"MouseEvents", "PointerUpdate", 581},
    {"Keys", "VK_ESCAPE", 27},
    {"Keys", "VK_PRIOR", 33},
    {"Keys", "VK_NEXT", 34},
    {"Keys", "VK_LEFT", 37},
    {"Keys", "VK_UP", 38},
    {"Keys", "VK_RIGHT", 39},
    {"Keys", "VK_DOWN", 40},
    {"Keys", "VK_NUMPAD1", 97},
    {"Keys", "VK_NUMPAD3", 99},
    {"Keys", "VK_NUMPAD4", 100},
    {"Keys", "VK_NUMPAD6", 102},
    {"Keys", "VK_NUMPAD7", 103},
    {"Keys", "VK_NUMPAD8", 104},
    {"Keys", "VK_OEM_PLUS", 187},
    {"Keys", "VK_OEM_MINUS", 189},
};
} // namespace civ5::runtime::detail

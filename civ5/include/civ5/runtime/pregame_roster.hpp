#pragma once
#include "civ5/content/rules_database.hpp"
#include "civ5/saves/pregame.hpp"
#include "civ5/runtime/lua_script_host.hpp"

namespace civ5::runtime {
struct PregameSlot {
    std::int64_t status=2, civilization=-1, leader=-1;
    bool minor=false;
    std::int64_t minor_type=-1;
    std::string leader_key;
    std::int64_t player_color=-1;
    std::string player_color_key;
};
// Owned session state, independent of a Lua context or a renderer. Sparse IDs
// and live getters allow a caller to share one roster without x86 slot limits.
struct PregameRoster {
    std::int64_t active_player=-1;
    std::map<std::int64_t,PregameSlot> slots;
};
// Slot status/active player come from the later archive. Civilization keys
// from slot hints bind to the activated database, not old numeric IDs.
PregameRoster restore_pregame_roster(const saves::PregameObservation&,const content::RulesDatabase&);
// The roster owner must outlive the host; changes are visible on the next call.
ScriptPregameProvider lua_pregame_services(const PregameRoster&);
// Mutating Lua pregame API shares the same roster used by gameplay and UI.
ScriptPregameProvider lua_pregame_services(PregameRoster&,const content::RulesDatabase&);
} // namespace civ5::runtime

#pragma once
#include "civ5/runtime/bnw_player_administration_checkpoint.hpp"
#include "civ5/runtime/live_session.hpp"

namespace civ5::runtime {

struct BnwPlayerAdministrationNotificationBundle {
    LiveSessionState state;
    std::size_t changed_notification_players=0;
    std::size_t changed_player_flags=0;
};

// Detached ID36 restore. Only the complete version-2 notification ring and
// the two raw, monotonic CvPlayer goody/builder history flags may change.
// The caller must construct a fresh UI notification owner after publication;
// this does not run Add/Dismiss broadcasts, Lua, DoGold or a research action.
// ID36 composition conflicts remain in force. Call after the other owned
// contexts have been restored and before publishing the LiveSession.
BnwPlayerAdministrationNotificationBundle
restore_bnw_player_administration_notification_bundle(
    const saves::CivisSaveSection &section,
    const LiveSessionState &baseline,
    const saves::SavedSession &source,
    const content::RulesDatabase &database,
    const SessionCheckpointIdentity &identity);

} // namespace civ5::runtime

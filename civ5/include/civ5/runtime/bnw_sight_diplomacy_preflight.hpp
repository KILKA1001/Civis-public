#pragma once
#include "civ5/runtime/bnw_sight_tech_share.hpp"

namespace civ5::runtime {

enum class BnwMeetDiplomacyCall : std::uint8_t {
    declare_war,
    proximity_our_to_their,
    proximity_their_to_our,
    first_contact,
    third_party_met_someone,
    clear_research_queue,
};
struct BnwMeetDiplomacyIntent {
    BnwMeetDiplomacyCall call=BnwMeetDiplomacyCall::declare_war;
    std::int64_t recipient_team=-1,other_team=-1;
    std::int64_t recipient_player=-1,other_player=-1,third_player=-1;
};
enum class BnwMeetDiplomacyBlocker : std::uint8_t {
    always_war_declare_war,
    proximity_and_first_contact_ai,
    research_queue_and_minor_effects,
    gameplay_ui_lua_and_persistence,
};
struct BnwSightDiplomacyPreflightResult {
    bool calculated=false;
    std::string reason;
    std::optional<LiveSessionState> detached;
    std::vector<BnwMeetDiplomacyIntent> intents;
    std::vector<BnwMeetDiplomacyBlocker> blockers;
    bool can_commit() const noexcept {return false;}
};

// Expansion2 CvTeam.cpp:3356-3458. Source-order intent trace after a
// no-grant updateTechShare. No declareWar, proximity, diplomacy, research
// queue, minor contact, UI or Lua callback is invoked. An always-war contact
// that requires declareWar stops at that earlier branch and discards the copy.
BnwSightDiplomacyPreflightResult preflight_bnw_sight_diplomacy(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity);
} // namespace civ5::runtime

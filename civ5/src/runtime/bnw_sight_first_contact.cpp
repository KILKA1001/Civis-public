#include "civ5/runtime/bnw_sight_first_contact.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
struct Failure : std::runtime_error {
    BnwSightFirstContactBlocker blocker;
    Failure(BnwSightFirstContactBlocker code,const char *why)
        :std::runtime_error(why),blocker(code) {}
};
void require(bool ok,BnwSightFirstContactBlocker code,const char *why) {
    if(!ok) throw Failure(code,why);
}
const saves::PlayerCoreField &field(const saves::SavedPlayerObservation &player,
                                     std::string_view key) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &candidate:player.core.fields)
        if(candidate.name && key==candidate.name) {
            require(!found,BnwSightFirstContactBlocker::city_roster_unavailable,
                    "BNW proximity player core has duplicate field");
            found=&candidate;
        }
    require(found,BnwSightFirstContactBlocker::city_roster_unavailable,
            "BNW proximity player core field is unavailable");
    return *found;
}
void check_cities(const LiveSessionState &state,std::int64_t id) {
    constexpr auto blocker=BnwSightFirstContactBlocker::city_roster_unavailable;
    require(state.cities.profile==content::TechnologyProfile::bnw &&
            state.world.all_players && id>=0 && id<63 &&
            state.world.all_players->players.size()==64 &&
            state.cities.players.contains(id),blocker,
            "BNW proximity owned city roster is unavailable");
    const auto &raw=state.world.all_players->players.at(
        static_cast<std::size_t>(id));
    const auto &live=state.cities.players.at(id);
    require(raw.index==static_cast<std::uint64_t>(id) &&
            raw.saved_player_id==id &&
            raw.cities.layout==saves::GameStateLayout::expansion2_version1 &&
            raw.cities.header.wire_city_count>=0 &&
            static_cast<std::uint64_t>(raw.cities.header.wire_city_count)==
                raw.cities.cities.size() &&
            live.cities.size()==raw.cities.cities.size(),blocker,
            "BNW proximity raw and owned city rosters disagree");
}
std::int64_t major_count(const LiveSessionState &state) {
    constexpr auto blocker=BnwSightFirstContactBlocker::city_roster_unavailable;
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.setup.players.size()==64 && state.world.pregame &&
            state.world.all_players &&
            state.world.all_players->players.size()==64,blocker,
            "BNW proximity player roster is unavailable");
    const auto minors=state.world.pregame->archive.flag_arrays.find(
        "minor_nation_civs");
    require(minors!=state.world.pregame->archive.flag_arrays.end() &&
            minors->second.size()>=63,blocker,
            "BNW proximity minor civilization flags are unavailable");
    std::int64_t count=0;
    // CvGame::countMajorCivsAlive scans MAX_CIV_PLAYERS, not MAX_MAJOR_CIVS.
    for(std::int64_t id=0;id<63;++id) {
        const auto setup=state.setup.players.find(id);
        require(setup!=state.setup.players.end(),blocker,
                "BNW proximity player setup is incomplete");
        const auto &raw=state.world.all_players->players.at(
            static_cast<std::size_t>(id));
        const auto &alive=field(raw,"m_bAlive");
        require(raw.saved_player_id==id && alive.values.size()==1 &&
                (alive.values[0]==0 || alive.values[0]==1) &&
                setup->second.alive==(alive.values[0]==1),blocker,
                "BNW proximity living major roster disagrees");
        if(setup->second.alive && !minors->second[static_cast<std::size_t>(id)])
            ++count;
    }
    return count;
}
void apply_zero_city_proximity(LiveSessionState &copy,
    const BnwMeetDiplomacyIntent &intent,std::int64_t majors,
    std::vector<BnwSightProximityChange> &changes) {
    constexpr auto roster=BnwSightFirstContactBlocker::city_roster_unavailable;
    const auto id=intent.recipient_player,other=intent.other_player;
    require(id>=0 && id<63 && other>=0 && other<63 && id!=other,roster,
            "BNW proximity player pair is invalid");
    check_cities(copy,id);check_cities(copy,other);
    const auto my_count=copy.cities.players.at(id).cities.size();
    const auto their_count=copy.cities.players.at(other).cities.size();
    require(my_count==0 || their_count==0,
            BnwSightFirstContactBlocker::city_distance_unimplemented,
            "BNW proximity city-pair distance and area branch is unavailable");
    auto &player=copy.world.all_players->players.at(static_cast<std::size_t>(id));
    saves::PlayerCoreField *proximity=nullptr;
    for(auto &candidate:player.core.fields)
        if(candidate.name && std::string_view(candidate.name)==
               "m_aiProximityToPlayer") {
            require(!proximity,roster,"BNW proximity field is duplicated");
            proximity=&candidate;
        }
    require(proximity &&
            proximity->encoding==saves::PlayerCoreEncoding::vector_i32 &&
            proximity->values.size()==64,roster,
            "BNW proximity saved vector is unavailable");
    // CvPlayer::DoUpdateProximityToPlayer starts at NO_PLAYER_PROXIMITY=-1.
    // CvGame's remaining-major clamps then select Close=2 or Far=1.
    const std::int64_t value=majors==2 ? 2 : majors<=4 ? 1 : -1;
    auto &slot=proximity->values.at(static_cast<std::size_t>(other));
    changes.push_back({id,other,slot,value});
    slot=value;
}
} // namespace

BnwSightFirstContactResult apply_bnw_sight_first_contact_detached(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwSightFirstContactResult result;
    auto earlier=preflight_bnw_sight_diplomacy(
        source,state,request,database,types,defines,identity);
    if(!earlier.calculated || !earlier.detached) {
        result.reason=earlier.reason.empty()
            ? "BNW proximity requires earlier meet preflight"
            : earlier.reason;
        return result;
    }
    try {
        auto &copy=*earlier.detached;
        const auto majors=major_count(copy);
        for(const auto &intent:earlier.intents) {
            switch(intent.call) {
            case BnwMeetDiplomacyCall::proximity_our_to_their:
            case BnwMeetDiplomacyCall::proximity_their_to_our:
                apply_zero_city_proximity(copy,intent,majors,
                                          result.proximity_changes);
                break;
            case BnwMeetDiplomacyCall::first_contact:
                result.blocker=
                    BnwSightFirstContactBlocker::first_contact_ai_unimplemented;
                result.reason="BNW first contact requires unported DiplomacyAI military and economic evaluation";
                result.detached=std::move(earlier.detached);
                result.calculated=true;
                return result;
            default:
                // Reaching later SDK calls without first-contact AI would
                // silently skip a mandatory earlier effect.
                throw Failure(BnwSightFirstContactBlocker::first_contact_ai_unimplemented,
                    "BNW meet reached later callback before first-contact AI");
            }
        }
        result.blocker=result.proximity_changes.empty()
            ? BnwSightFirstContactBlocker::none
            : BnwSightFirstContactBlocker::player_core_checkpoint_unimplemented;
        result.reason=result.proximity_changes.empty()
            ? "BNW meet has no new contact proximity pair"
            : "BNW player proximity lacks a player-core checkpoint";
        result.detached=std::move(earlier.detached);
        result.calculated=true;
    } catch(const Failure &error) {
        result.blocker=error.blocker;
        result.reason=error.what();
        result.proximity_changes.clear();
    } catch(const std::exception &error) {
        result.blocker=BnwSightFirstContactBlocker::earlier_preflight_rejected;
        result.reason=error.what();
        result.proximity_changes.clear();
    }
    return result;
}
} // namespace civ5::runtime

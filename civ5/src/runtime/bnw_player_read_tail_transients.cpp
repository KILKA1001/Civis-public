#include "civ5/runtime/bnw_player_read_tail_transients.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
std::int64_t core_scalar(const saves::PlayerCoreObservation &core,
                         std::string_view wanted) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"BNW player Read tail has duplicate core scalar");
            found=&field;
        }
    require(found && found->values.size()==1,
            "BNW player Read tail core scalar missing or changed shape");
    return found->values[0];
}
} // namespace

BnwPlayerReadTailTransients rebuild_bnw_player_read_tail_transients(
    const LiveSessionState &state,
    const saves::SavedSession &source,
    const SessionCheckpointIdentity &identity) {
    constexpr std::uint64_t players=64,major=22;
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==
                saves::GameStateLayout::expansion2_version1 &&
            identify_bytes(source.source.bytes).sha256==identity.payload_sha256 &&
            state.world.all_players && source.snapshot.all_players &&
            state.world.all_players->players.size()==players &&
            source.snapshot.all_players->players.size()==players &&
            state.world.all_players->context.player_count==players &&
            state.world.all_players->context.strategy.major==major &&
            source.snapshot.all_players->context.strategy.major==major &&
            state.setup.players.size()==players,
            "BNW player Read tail source, profile or fixed roster changed");
    BnwPlayerReadTailTransients result;
    for(std::size_t i=0;i<players;++i) {
        const auto &saved=source.snapshot.all_players->players[i];
        const auto &current=state.world.all_players->players[i];
        require(saved.index==i && current.index==i &&
                saved.saved_player_id==static_cast<std::int64_t>(i) &&
                current.saved_player_id==static_cast<std::int64_t>(i) &&
                saved.core.layout==saves::GameStateLayout::expansion2_version1 &&
                current.core.layout==saved.core.layout &&
                saved.core.start_offset==current.core.start_offset &&
                saved.core.end_offset==current.core.end_offset &&
                state.setup.players.contains(i) &&
                core_scalar(saved.core,"m_eID")==static_cast<std::int64_t>(i) &&
                core_scalar(current.core,"m_eID")==static_cast<std::int64_t>(i),
                "BNW player Read tail owner identity changed");
        if(i<major) {
            // CvDiplomacyRequests::Init -> Uninit, then owner assignment.
            // The BNW CvPlayer::Read request deserialization is commented out.
            result.requests.emplace(static_cast<std::int64_t>(i),
                BnwDiplomacyRequestsAfterRead{static_cast<std::int64_t>(i),-1,
                                              false,-1,0});
        }
        const auto alive=core_scalar(current.core,"m_bAlive");
        const auto active=core_scalar(current.core,"m_bTurnActive");
        require((alive==0 || alive==1) && (active==0 || active==1) &&
                state.setup.players.at(i).alive==(alive!=0),
                "BNW player Read tail raw and owned turn roster diverged");
        // CvGame::getNumGameTurnActive loops MAX_PLAYERS and counts only
        // alive && turnActive. changeNumGameTurnActive does not store a count.
        if(alive && active)
            result.active_player_ids.push_back(static_cast<std::int64_t>(i));
    }
    result.current=true;
    return result;
}

} // namespace civ5::runtime

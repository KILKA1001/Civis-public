#include "civ5/runtime/bnw_player_pre_turn.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/content/rules_database.hpp"
#include <string>
#include <string_view>
#include <utility>

namespace civ5::runtime {
namespace {
using B=BnwPlayerPreTurnBlocker;
void require(bool ok,B blocker,const char *message) {
    if(!ok) throw BnwPlayerPreTurnError(blocker,message);
}
std::string_view key(std::string_view wire) {
    return wire.substr(0,wire.find('\0'));
}
std::string_view slot(const saves::PregameObservation &pregame,
                      std::string_view name,std::int64_t player_id) {
    const auto found=pregame.slots.string_arrays.find(std::string(name));
    require(found!=pregame.slots.string_arrays.end() && player_id>=0 &&
            static_cast<std::uint64_t>(player_id)<found->second.size(),
            B::pregame_identity,"BNW player pre-turn slot unavailable");
    return key(found->second[static_cast<std::size_t>(player_id)]);
}
}

void preflight_bnw_player_cache_and_ai_pre(
    const LiveSessionState &state,std::int64_t player_id,
    const content::RulesDatabase &rules,
    const SessionCheckpointIdentity &identity) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==bnw &&
            state.world.pregame && state.world.pregame->layout==bnw &&
            state.world.all_players && state.world.all_players->layout==bnw,
            B::profile,"BNW player pre-turn requires restored Expansion2 world");
    require(!identity.rules_sha256.empty() &&
            identify_bytes(rules.gameplay_fingerprint_data()).sha256==identity.rules_sha256,
            B::selected_rules,"BNW player pre-turn selected rules differ");
    const auto setup=state.setup.players.find(player_id);
    require(setup!=state.setup.players.end() && setup->second.alive,
            B::player,"BNW player pre-turn requires alive player");
    const saves::SavedPlayerObservation *saved=nullptr;
    for(const auto &candidate:state.world.all_players->players)
        if(candidate.saved_player_id==player_id) {
            require(!saved,B::player,"duplicate BNW player pre-turn ID");
            saved=&candidate;
        }
    require(saved && saved->core.layout==bnw,B::player,
            "BNW player pre-turn saved player unavailable");
    const auto leader=slot(*state.world.pregame,"leader_keys",player_id);
    const auto civilization=slot(*state.world.pregame,"civilization_keys",player_id);
    require(!leader.empty() && !civilization.empty(),B::pregame_identity,
            "BNW AI pre-turn leader or civilization unresolved");
    bool personality=false;
    for(const auto &field:saved->core.fields)
        if(field.name && std::string_view(field.name)=="m_ePersonalityType") {
            require(!personality && field.encoding==saves::PlayerCoreEncoding::i32 &&
                    field.values.size()==1 && field.values[0]>=0,
                    B::pregame_identity,"BNW AI pre-turn personality invalid");
            personality=true;
        }
    require(personality,B::pregame_identity,
            "BNW AI pre-turn personality unavailable");
    const auto turn=state.research_turn.players.find(player_id);
    require(turn!=state.research_turn.players.end() && turn->second.leader_type &&
            key(*turn->second.leader_type)==leader,
            B::pregame_identity,"BNW AI pre-turn live leader differs");
    bool human=false;
    try {human=state.setup.is_human(setup->second);}
    catch(const std::exception &) {
        throw BnwPlayerPreTurnError(B::player,
                                    "BNW player pre-turn human status unresolved");
    }
    require(human,B::ai_pre_effects,
            "BNW AI pre-turn found-value, research and annex effects unavailable");
}

BnwPlayerEarlyTurnPrefix apply_bnw_human_player_early_turn(
    LiveSessionState &state,std::int64_t player_id,
    std::int64_t active_player_id,
    const content::RulesDatabase &rules,
    const SessionCheckpointIdentity &identity) {
    preflight_bnw_player_cache_and_ai_pre(state,player_id,rules,identity);
    return apply_bnw_player_early_turn_prefix(
        state,player_id,active_player_id,rules,identity);
}

std::array<BnwTurnHandler,3> make_bnw_human_player_early_turn_handlers(
    const content::RulesDatabase &rules,
    SessionCheckpointIdentity identity,std::int64_t active_player_id,
    BnwPlayerEarlyTurnServices services) {
    const auto *selected_rules=&rules;
    auto cache=[](LiveSessionState &state,
                  std::int64_t player)->LiveActionResult {
        const auto found=state.setup.players.find(player);
        if(state.setup.profile!=content::TechnologyProfile::bnw ||
           state.world.game_state.layout!=saves::GameStateLayout::expansion2_version1 ||
           found==state.setup.players.end() || !found->second.alive)
            return {LiveActionOutcome::rejected,
                    "BNW player cache requires a living Expansion2 player"};
        // CvPlayer::doUpdateCacheOnTurn has an empty body in Expansion2.
        return {LiveActionOutcome::committed,{}};
    };
    auto ai_pre=[selected_rules,identity](LiveSessionState &state,
                                           std::int64_t player)->LiveActionResult {
        try {
            preflight_bnw_player_cache_and_ai_pre(
                state,player,*selected_rules,identity);
            return {LiveActionOutcome::committed,{}};
        } catch(const std::exception &error) {
            return {LiveActionOutcome::rejected,error.what()};
        }
    };
    return {{
        {BnwTurnPhase::player_cache,cache},
        {BnwTurnPhase::player_ai_pre,ai_pre},
        {BnwTurnPhase::player_timers_and_revolts,
         [selected_rules,identity=std::move(identity),active_player_id,services](
             LiveSessionState &state,std::int64_t player)->LiveActionResult {
             try {
                 if(state.player_early_publication)
                     return {LiveActionOutcome::rejected,
                             "BNW early player UI publication is still pending"};
                 auto draft=state;
                 const auto step=apply_bnw_human_player_early_turn(
                     draft,player,active_player_id,*selected_rules,identity);
                 if(const auto &dirty=step.initial.scalars.conversion.post_commit_dirty) {
                     if(!services.dirty_after_commit)
                         return {LiveActionOutcome::rejected,
                             "BNW conversion timer UI dirty publication unavailable"};
                     draft.player_early_publication=BnwPlayerEarlyPublication{
                         dirty->player_id,static_cast<std::uint8_t>(dirty->bit),false};
                 }
                 state=std::move(draft);
                 return {LiveActionOutcome::committed,{}};
             } catch(const std::exception &error) {
                 return {LiveActionOutcome::rejected,error.what()};
             }
         },
         [services=std::move(services)](LiveSession &session) {
             return publish_bnw_player_early_turn(session,services);
         }}
    }};
}

LiveActionResult publish_bnw_player_early_turn(
    LiveSession &session,const BnwPlayerEarlyTurnServices &services) {
    const auto &pending=session.state().player_early_publication;
    if(!pending) return {LiveActionOutcome::committed,{}};
    if(!services.dirty_after_commit || pending->uncertain ||
       pending->dirty_kind!=static_cast<std::uint8_t>(
           BnwConversionTimerDirtyBit::misc_buttons))
        return {LiveActionOutcome::rejected,
                "BNW early player UI publication unavailable or uncertain"};
    const auto player=pending->player;
    const auto kind=static_cast<BnwConversionTimerDirtyBit>(pending->dirty_kind);
    session.mutable_state().player_early_publication->uncertain=true;
    try {
        services.dirty_after_commit(player,kind);
    } catch(const std::exception &error) {
        return {LiveActionOutcome::committed,
                std::string("BNW early player UI publication uncertain: ")+error.what()};
    } catch(...) {
        return {LiveActionOutcome::committed,
                "BNW early player UI publication uncertain"};
    }
    session.mutable_state().player_early_publication.reset();
    return {LiveActionOutcome::committed,{}};
}
} // namespace civ5::runtime

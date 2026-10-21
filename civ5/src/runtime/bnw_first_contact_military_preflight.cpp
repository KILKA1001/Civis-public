#include "civ5/runtime/bnw_first_contact_military_preflight.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>
#include <variant>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
struct Failure : std::runtime_error {
    BnwFirstContactMilitaryGate gate;
    Failure(BnwFirstContactMilitaryGate value,const char *why)
        :std::runtime_error(why),gate(value) {}
};
void require(bool ok,BnwFirstContactMilitaryGate gate,const char *why) {
    if(!ok) throw Failure(gate,why);
}
std::int64_t scalar(const saves::SavedPlayerObservation &player,
                    std::string_view wanted) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:player.core.fields)
        if(field.name && wanted==field.name) {
            require(!found,BnwFirstContactMilitaryGate::clock_or_roster_inconsistent,
                    "BNW first-contact player core field is duplicated");
            found=&field;
        }
    require(found &&
            (found->encoding==saves::PlayerCoreEncoding::i32 ||
             (wanted=="m_bAlive" &&
              found->encoding==saves::PlayerCoreEncoding::boolean)) &&
            found->values.size()==1,
            BnwFirstContactMilitaryGate::clock_or_roster_inconsistent,
            "BNW first-contact might cache field is unavailable");
    return found->values[0];
}
void validate_player(const LiveSessionState &state,std::int64_t id) {
    constexpr auto gate=BnwFirstContactMilitaryGate::clock_or_roster_inconsistent;
    require(id>=0 && id<63 && state.world.all_players &&
            state.world.all_players->layout==bnw &&
            state.world.all_players->players.size()==64 &&
            state.setup.profile==content::TechnologyProfile::bnw &&
            state.setup.players.contains(id) &&
            state.cities.profile==content::TechnologyProfile::bnw &&
            state.cities.players.contains(id),gate,
            "BNW first-contact player owner is unavailable");
    const auto &raw=state.world.all_players->players.at(
        static_cast<std::size_t>(id));
    const auto &owned=state.cities.players.at(id);
    require(raw.index==static_cast<std::uint64_t>(id) &&
            raw.saved_player_id==id && raw.core.layout==bnw &&
            raw.cities.layout==bnw &&
            scalar(raw,"m_bAlive")==1 &&
            state.setup.players.at(id).alive &&
            raw.cities.header.wire_city_count>=0 &&
            static_cast<std::uint64_t>(raw.cities.header.wire_city_count)==
                raw.cities.cities.size() &&
            owned.cities.size()==raw.cities.cities.size(),gate,
            "BNW first-contact player alive or city roster disagrees");
}
std::int64_t elapsed(const LiveSessionState &state) {
    constexpr auto gate=BnwFirstContactMilitaryGate::clock_or_roster_inconsistent;
    require(state.world.game_state.layout==bnw &&
            state.maintenance.profile==content::TechnologyProfile::bnw &&
            std::holds_alternative<saves::Expansion2PrefixObservation>(
                state.world.game_state.prefix),gate,
            "BNW first-contact elapsed clock profile is unavailable");
    const auto &prefix=std::get<saves::Expansion2PrefixObservation>(
        state.world.game_state.prefix);
    require(prefix.scalars.size()>1 && prefix.scalars[1]>=0 &&
            prefix.scalars[1]<=INT32_MAX &&
            state.maintenance.elapsed_turns==prefix.scalars[1],gate,
            "BNW first-contact elapsed clock mirrors disagree");
    return prefix.scalars[1];
}
void define_base(const content::GlobalDefineCache &defines) {
    constexpr auto gate=BnwFirstContactMilitaryGate::military_rules_unavailable;
    const auto *value=defines.find("MILITARY_STRENGTH_BASE");
    require(defines.profile()==content::GlobalDefineProfile::bnw &&
            value && value->lookup.found && value->lookup.sqlite_code==0 &&
            std::holds_alternative<std::int64_t>(value->lookup.value) &&
            std::get<std::int64_t>(value->lookup.value)>=0 &&
            std::get<std::int64_t>(value->lookup.value)<=INT32_MAX,gate,
            "BNW first-contact military base is missing from active rules");
}
} // namespace

BnwFirstContactMilitaryPreflight preflight_bnw_first_contact_military(
    const saves::SavedSession &source,const LiveSessionState &state,
    const BnwSetXYRequest &request,const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwFirstContactMilitaryPreflight result;
    auto contact=apply_bnw_sight_first_contact_detached(
        source,state,request,database,types,defines,identity);
    if(!contact.calculated || !contact.detached ||
       contact.blocker!=BnwSightFirstContactBlocker::first_contact_ai_unimplemented ||
       contact.proximity_changes.size()!=2 ||
       contact.proximity_changes[1].player!=contact.proximity_changes[0].other ||
       contact.proximity_changes[1].other!=contact.proximity_changes[0].player) {
        result.reason=contact.reason.empty()
            ? "BNW first-contact military requires ordered detached meet and proximity"
            : contact.reason;
        return result;
    }
    result.source_verified=true;
    result.recipient=contact.proximity_changes[0].player;
    result.target=contact.proximity_changes[0].other;
    try {
        const auto &copy=*contact.detached;
        validate_player(copy,result.recipient);
        validate_player(copy,result.target);
        define_base(defines); // CvDiplomacyAI.cpp:6409 precedes GetMilitaryMight.
        result.elapsed_turns=elapsed(copy);
        const auto &own=copy.world.all_players->players.at(
            static_cast<std::size_t>(result.recipient));
        result.own_might_cache_turn=scalar(own,"m_iTurnMightRecomputed");
        require(result.own_might_cache_turn<=result.elapsed_turns,
                BnwFirstContactMilitaryGate::clock_or_roster_inconsistent,
                "BNW first-contact own might cache is from a future turn");
        if(result.own_might_cache_turn<result.elapsed_turns) {
            result.gate=BnwFirstContactMilitaryGate::own_might_lazy_recompute;
            result.reason="BNW GetMilitaryMight(own) must recompute unit power and economic might before target validity";
            return result;
        }
        (void)scalar(own,"m_iMilitaryMight");
        result.target_city_count=copy.world.all_players->players.at(
            static_cast<std::size_t>(result.target))
                .cities.header.wire_city_count;
        if(result.target_city_count==0) {
            // IsPlayerValid returns before has_met and target GetMilitaryMight.
            result.first_call_completed=true;
            result.gate=BnwFirstContactMilitaryGate::next_economic_ai_unimplemented;
            result.reason="BNW first-contact military assessment skips target without cities; economic AI is next";
            return result;
        }
        const auto own_team=copy.setup.players.at(result.recipient).team;
        const auto target_team=copy.setup.players.at(result.target).team;
        require(own_team>=0 && own_team<63 && target_team>=0 &&
                target_team<63 && copy.world.teams &&
                copy.world.teams->teams.size()==64,
                BnwFirstContactMilitaryGate::clock_or_roster_inconsistent,
                "BNW first-contact team roster is unavailable");
        const auto &team=copy.world.teams->teams.at(
            static_cast<std::size_t>(own_team));
        const saves::PlotArrayObservation *met=nullptr;
        for(const auto &candidate:team.arrays)
            if(candidate.name && std::string_view(candidate.name)=="has_met") {
                require(!met,
                    BnwFirstContactMilitaryGate::clock_or_roster_inconsistent,
                    "BNW first-contact has_met array is duplicated");
                met=&candidate;
            }
        require(met && met->columns==1 && met->values.size()==64 &&
                met->values.at(static_cast<std::size_t>(target_team))==1,
                BnwFirstContactMilitaryGate::clock_or_roster_inconsistent,
                "BNW first-contact target is not met in owned team state");
        const auto &target=copy.world.all_players->players.at(
            static_cast<std::size_t>(result.target));
        result.target_might_cache_turn=scalar(target,"m_iTurnMightRecomputed");
        require(result.target_might_cache_turn<=result.elapsed_turns,
                BnwFirstContactMilitaryGate::clock_or_roster_inconsistent,
                "BNW first-contact target might cache is from a future turn");
        if(result.target_might_cache_turn<result.elapsed_turns) {
            result.gate=BnwFirstContactMilitaryGate::target_might_lazy_recompute;
            result.reason="BNW GetMilitaryMight(target) must recompute unit power and economic might";
            return result;
        }
        (void)scalar(target,"m_iMilitaryMight");
        result.gate=BnwFirstContactMilitaryGate::military_assessment_unimplemented;
        result.reason="BNW first-contact military ratio and diplomacy field update remain unported";
    } catch(const Failure &error) {
        result.gate=error.gate;
        result.reason=error.what();
    } catch(const std::exception &error) {
        result.gate=BnwFirstContactMilitaryGate::clock_or_roster_inconsistent;
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime

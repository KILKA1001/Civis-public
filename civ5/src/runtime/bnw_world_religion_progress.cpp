#include "civ5/runtime/bnw_world_religion_progress.hpp"
#include "civ5/runtime/bnw_game_score.hpp"
#include <algorithm>
#include <climits>
#include <exception>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace civ5::runtime {
namespace {
void require(bool condition,const char *why) {
    if(!condition) throw std::runtime_error(why);
}
std::int64_t field(const std::vector<saves::NamedSaveInteger> &fields,
                   std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &entry:fields)
        if(entry.name && std::string_view(entry.name)==wanted) {
            require(!found,"Duplicate BNW religion progress scalar");
            found=&entry;
        }
    require(found,"Missing BNW religion progress scalar");
    return found->value;
}
std::int64_t checked_sum(std::int64_t left,std::int64_t right) {
    require(left>=0 && right>=0 && left<=INT32_MAX-right,
            "BNW religion tech cost exceeds source i32 range");
    return left+right;
}
std::int64_t research_left(std::int64_t tech,
    const TeamTechnologies &team,const content::TechnologyRules &rules,
    std::int64_t and_slots,std::vector<std::int64_t> &ancestors) {
    if(tech<0) return 0;
    const auto *info=rules.find(static_cast<std::uint64_t>(tech));
    if(!info) return 0; // CvGame::GetResearchLeftToTech missing Info branch.
    const auto cost=info->integers.find("Cost");
    require(cost!=info->integers.end() && cost->second>=0 &&
            cost->second<=INT32_MAX,"BNW religion technology cost unavailable");
    const auto prerequisite=[&](std::int64_t slot) {
        return static_cast<std::uint64_t>(slot)<info->and_prerequisites.extent
            ?info->and_prerequisites.at(static_cast<std::uint64_t>(slot)):-1;
    };
    // Preserve source order: a root tech returns full Cost even when owned.
    if(prerequisite(0)==-1) return cost->second;
    if(team.has_tech(tech)) return 0;
    require(std::find(ancestors.begin(),ancestors.end(),tech)==ancestors.end(),
            "BNW religion technology prerequisites contain a cycle");
    ancestors.push_back(tech);
    auto total=cost->second;
    for(std::int64_t slot=0;slot<and_slots;++slot) {
        const auto next=prerequisite(slot);
        if(next!=-1)
            total=checked_sum(total,research_left(next,team,rules,and_slots,ancestors));
    }
    ancestors.pop_back();
    return total;
}
bool is_minor_team(const LiveSessionState &state,std::int64_t team_id,
                   const std::vector<bool> &minor_flags) {
    // CvTeam::isMinorCiv scans players [MAX_MAJOR_CIVS,MAX_PLAYERS) and
    // returns at the first living member of this team.
    for(std::int64_t player=22;player<64;++player) {
        const auto found=state.setup.players.find(player);
        require(found!=state.setup.players.end(),
                "BNW religion progress player roster incomplete");
        if(found->second.alive && found->second.team==team_id)
            return minor_flags.at(static_cast<std::size_t>(player));
    }
    return false;
}
} // namespace

std::int32_t compute_bnw_world_religion_tech_progress(
    const LiveSessionState &state,const content::TechnologyRules &technologies,
    std::int64_t and_prerequisite_slots) {
    constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
    const auto &world=state.world;
    require(world.game_state.layout==bnw && world.teams &&
            world.teams->layout==bnw && world.pregame &&
            world.pregame->layout==bnw &&
            state.setup.profile==content::TechnologyProfile::bnw &&
            technologies.profile==content::TechnologyProfile::bnw &&
            technologies.all_succeeded() &&
            world.teams->dimensions.civ_teams==63 &&
            world.teams->teams.size()==64 &&
            state.setup.players.size()==64 &&
            and_prerequisite_slots>0 && and_prerequisite_slots<=1024,
            "BNW religion progress requires complete matching rules and roster");
    const auto target=field(world.game_state.controls,"religion_tech");
    if(target==-1) return -1;
    require(target>=0,"BNW religion technology ID invalid");
    const auto minor=world.pregame->archive.flag_arrays.find("minor_nation_civs");
    require(minor!=world.pregame->archive.flag_arrays.end() &&
            minor->second.size()>=64,
            "BNW religion progress minor-civilization flags unavailable");
    std::int64_t count=0,total=0;
    for(std::int64_t id=0;id<63;++id) {
        const auto &saved=world.teams->teams.at(static_cast<std::size_t>(id));
        require(field(saved.fields,"id")==id,
                "BNW religion progress team ID mismatch");
        const auto alive=field(saved.fields,"alive_count");
        require(alive>=0,"BNW religion progress team alive count invalid");
        if(!alive || is_minor_team(state,id,minor->second)) continue;
        const auto owned=std::find_if(state.technologies.begin(),
            state.technologies.end(),[&](const TeamTechnologies &team) {
                return team.team()==id && team.profile()==
                    content::TechnologyProfile::bnw;
            });
        require(owned!=state.technologies.end(),
                "BNW religion progress owned team technologies unavailable");
        std::vector<std::int64_t> ancestors;
        total=checked_sum(total,research_left(target,*owned,technologies,
                                              and_prerequisite_slots,ancestors));
        ++count;
    }
    require(count>0,"BNW religion progress has no living major team");
    return static_cast<std::int32_t>(total/count);
}

void update_bnw_world_religion_tech_progress(
    LiveSessionState &state,const content::TechnologyRules &technologies,
    std::int64_t and_prerequisite_slots) {
    const auto value=compute_bnw_world_religion_tech_progress(
        state,technologies,and_prerequisite_slots);
    require(std::holds_alternative<saves::Expansion2PrefixObservation>(
                state.world.game_state.prefix),
            "BNW religion progress prefix unavailable");
    std::get<saves::Expansion2PrefixObservation>(
        state.world.game_state.prefix).scalars[23]=value;
}

BnwTurnHandler make_bnw_game_caches_and_score_handler(
    const content::TechnologyRules &technologies,
    std::int64_t and_prerequisite_slots) {
    return {BnwTurnPhase::game_caches_and_score,
            [&technologies,and_prerequisite_slots](LiveSessionState &state,
                                                   std::int64_t player) {
                if(player!=-1)
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            "BNW cache and score is a game phase"};
                if(state.world.game_state.layout!=
                   saves::GameStateLayout::expansion2_version1)
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            "BNW cache and score requires BNW Game state"};
                const saves::NamedSaveFlag *dirty=nullptr;
                for(const auto &flag:state.world.game_state.flags) {
                    if(!flag.name || std::string_view(flag.name)!="score_dirty")
                        continue;
                    if(dirty)
                        return LiveActionResult{LiveActionOutcome::rejected,
                                                "Duplicate BNW score dirty flag"};
                    dirty=&flag;
                }
                if(!dirty)
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            "Missing BNW score dirty flag"};
                if(dirty->value)
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            "BNW score ranking and replay update unavailable"};
                try {
                    update_bnw_world_religion_tech_progress(
                        state,technologies,and_prerequisite_slots);
                } catch(const std::exception &error) {
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            std::string("BNW religion cache: ")+error.what()};
                }
                return LiveActionResult{LiveActionOutcome::committed,{}};
            }};
}

BnwTurnHandler make_bnw_game_caches_and_score_handler(
    const content::TechnologyRules &technologies,
    std::int64_t and_prerequisite_slots,
    const content::RulesDatabase &rules,
    const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines) {
    return {BnwTurnPhase::game_caches_and_score,
            [&technologies,and_prerequisite_slots,&rules,&types,&defines](
                LiveSessionState &state,std::int64_t player) {
                if(player!=-1)
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            "BNW cache and score is a game phase"};
                if(state.world.game_state.layout!=
                   saves::GameStateLayout::expansion2_version1)
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            "BNW cache and score requires BNW Game state"};
                const saves::NamedSaveFlag *dirty=nullptr;
                for(const auto &flag:state.world.game_state.flags) {
                    if(!flag.name || std::string_view(flag.name)!="score_dirty")
                        continue;
                    if(dirty)
                        return LiveActionResult{LiveActionOutcome::rejected,
                                                "Duplicate BNW score dirty flag"};
                    dirty=&flag;
                }
                if(!dirty)
                    return LiveActionResult{LiveActionOutcome::rejected,
                                            "Missing BNW score dirty flag"};
                try {
                    if(dirty->value) {
                        // Both SDK calls form one phase. The detached candidate
                        // keeps direct handler use atomic on an input error.
                        auto candidate=state;
                        update_bnw_world_religion_tech_progress(
                            candidate,technologies,and_prerequisite_slots);
                        update_bnw_game_score(candidate,rules,types,defines);
                        state=std::move(candidate);
                    } else {
                        update_bnw_world_religion_tech_progress(
                            state,technologies,and_prerequisite_slots);
                    }
                } catch(const std::exception &error) {
                    return LiveActionResult{LiveActionOutcome::rejected,
                        std::string("BNW game cache and score: ")+error.what()};
                }
                return LiveActionResult{LiveActionOutcome::committed,{}};
            }};
}

} // namespace civ5::runtime

#include "civ5/runtime/bnw_team_turn_pair.hpp"
#include <climits>
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr std::size_t team_count=64;
void require(bool value,const char *message) {
    if(!value) throw std::runtime_error(message);
}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
const saves::PlotArrayObservation &array(const saves::TeamObservation &team,
                                         std::string_view wanted) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &item:team.arrays) if(name(item.name)==wanted) {
        require(!found,"Duplicate BNW team turn technology array");
        found=&item;
    }
    require(found && found->columns==1,"Missing BNW team turn technology array");
    return *found;
}
void compare_owned_to_raw(std::span<const TeamTechnologies> live,
                          std::span<const TeamTechnologies> baseline,
                          const saves::SavedSessionSnapshot &world,
                          const saves::SavedSessionSnapshot &source,
                          const content::TechnologyRules &rules,
                          const content::InfoTypeCatalog &types) {
    require(rules.profile==content::TechnologyProfile::bnw &&
            rules.all_succeeded() && types.report().all_succeeded() &&
            source.teams && world.teams &&
            source.teams->teams.size()==team_count &&
            world.teams->teams.size()==team_count &&
            live.size()==team_count && baseline.size()==team_count,
            "BNW team turn rules or team roster changed");
    const auto decoded=restore_team_technologies(*world.teams,rules,types);
    const auto original=restore_team_technologies(*source.teams,rules,types);
    require(decoded.size()==team_count && original.size()==team_count,
            "BNW team turn decoded roster changed");
    for(std::size_t team=0;team<team_count;++team) {
        const auto &a=live[team],&b=baseline[team],&c=decoded[team],&d=original[team];
        require(a.profile()==rules.profile && b.profile()==rules.profile &&
                a.team()==static_cast<std::int64_t>(team) &&
                b.team()==a.team() && c.team()==a.team() && d.team()==a.team() &&
                a.slots()==rules.collection.slots && b.slots()==a.slots() &&
                c.slots()==a.slots() && d.slots()==a.slots() &&
                a.remap().all_mapped() && b.remap().all_mapped() &&
                c.remap().all_mapped() && d.remap().all_mapped() &&
                a.remap().duplicates==0 && b.remap().duplicates==0 &&
                c.remap().duplicates==0 && d.remap().duplicates==0 &&
                a.remap().holes==0 && b.remap().holes==0 &&
                c.remap().holes==0 && d.remap().holes==0 &&
                a.profile_extensions().values.empty() &&
                b.profile_extensions().values.empty() &&
                c.profile_extensions().values.empty() &&
                d.profile_extensions().values.empty() &&
                a.last_tech()==b.last_tech() &&
                a.last_tech()==c.last_tech() && b.last_tech()==d.last_tech(),
                "BNW team turn owned identity or last technology changed");
        const auto &hashes=array(source.teams->teams[team],"tech_hashes").values;
        const auto &old_flags=array(source.teams->teams[team],"no_trade_tech").values;
        const auto &new_flags=array(world.teams->teams[team],"no_trade_tech").values;
        require(hashes.size()==old_flags.size() && hashes.size()==new_flags.size(),
                "BNW team turn technology row count changed");
        std::set<std::int64_t> seen;
        for(std::size_t row=0;row<hashes.size();++row) {
            require(old_flags[row]>=0 && old_flags[row]<=1 &&
                    new_flags[row]>=0 && new_flags[row]<=1,
                    "Invalid BNW team turn no-trade flag");
            if(hashes[row]==0) {
                require(old_flags[row]==0 && new_flags[row]==0,
                        "Unmapped BNW team turn no-trade flag");
                continue;
            }
            require(hashes[row]>0 && hashes[row]<=UINT32_MAX,
                    "Invalid BNW team turn technology hash");
            const auto *entry=types.find_hash(static_cast<std::uint64_t>(hashes[row]));
            require(entry && entry->id>=0 &&
                    static_cast<std::uint64_t>(entry->id)<a.slots() &&
                    rules.find(static_cast<std::uint64_t>(entry->id)) &&
                    seen.insert(entry->id).second,
                    "Ambiguous BNW team turn technology row");
            require(b.at(entry->id).no_trade==(old_flags[row]!=0) &&
                    a.at(entry->id).no_trade==(new_flags[row]!=0),
                    "BNW team turn raw/owned no-trade mismatch");
        }
        for(std::uint64_t slot=0;slot<a.slots();++slot) {
            const auto id=static_cast<std::int64_t>(slot);
            const auto av=a.at(id),bv=b.at(id);
            require(av==c.at(id) && bv==d.at(id) &&
                    av.has_tech==bv.has_tech &&
                    av.progress_times100==bv.progress_times100 &&
                    av.count==bv.count &&
                    (av.no_trade==bv.no_trade || seen.contains(id)),
                    "BNW team turn changed another owned technology field");
        }
    }
}
void compare_other_id8(const ResearchContext &setup,
                       std::span<const TeamTechnologies> baseline_technologies,
                       const PlayerResearchContext &baseline_players,
                       const PlayerResearchContext &live_players,
                       const content::TechnologyRules &rules,
                       const SessionCheckpointIdentity &identity) {
    // Both calls use identical setup and technologies. Equal complete ID8
    // payloads therefore prove that its remaining player fields are unchanged.
    const auto baseline=make_research_checkpoint(baseline_technologies,
        baseline_players,setup,rules,identity);
    const auto live=make_research_checkpoint(baseline_technologies,
        live_players,setup,rules,identity);
    require(!baseline.profile && !live.profile &&
            baseline.common.id==live.common.id &&
            baseline.common.flags==live.common.flags &&
            baseline.common.bytes==live.common.bytes,
            "BNW team turn changed another owned ID8 field");
}
} // namespace

BnwTeamTurnCheckpointPair make_bnw_team_turn_checkpoint_pair(
    const LiveSessionState &state,
    const saves::SavedSessionSnapshot &source,
    std::span<const TeamTechnologies> baseline_technologies,
    const PlayerResearchContext &baseline_players,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity) {
    compare_owned_to_raw(state.technologies,baseline_technologies,
        state.world,source,rules,types);
    compare_other_id8(state.setup,baseline_technologies,baseline_players,
        state.player_research,rules,identity);
    auto raw=make_bnw_team_turn_checkpoint(state.world,source,identity);
    auto owned=make_research_checkpoint(state.technologies,
        state.player_research,state.setup,rules,identity);
    require(!owned.profile,"BNW team turn received profile research section");
    return {std::move(raw),std::move(owned.common)};
}

BnwTeamTurnRestored restore_bnw_team_turn_checkpoint_pair(
    const BnwTeamTurnCheckpointPair &pair,
    const saves::SavedSessionSnapshot &source,
    const ResearchContext &setup,
    std::span<const TeamTechnologies> baseline_technologies,
    const PlayerResearchContext &baseline_players,
    const content::TechnologyRules &rules,
    const content::InfoTypeCatalog &types,
    const SessionCheckpointIdentity &identity) {
    BnwTeamTurnRestored result{source,
        {baseline_technologies.begin(),baseline_technologies.end()},baseline_players};
    restore_bnw_team_turn_checkpoint(pair.raw,result.world,source,identity);
    restore_research_checkpoint(pair.owned,std::nullopt,result.technologies,
        result.player_research,setup,rules,identity);
    compare_other_id8(setup,baseline_technologies,baseline_players,
        result.player_research,rules,identity);
    compare_owned_to_raw(result.technologies,baseline_technologies,
        result.world,source,rules,types);
    return result;
}

} // namespace civ5::runtime

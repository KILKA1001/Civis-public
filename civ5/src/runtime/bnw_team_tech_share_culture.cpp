#include "civ5/runtime/bnw_team_tech_share_culture.hpp"
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool condition,const char *reason) {
    if(!condition) throw std::runtime_error(reason);
}
template<class T> const T &named(const std::vector<T> &items,std::string_view name) {
    const T *found=nullptr;
    for(const auto &item:items)
        if(item.name && name==item.name) {
            require(!found,"Duplicate BNW tech-share or city field");
            found=&item;
        }
    require(found,"Missing BNW tech-share or city field");
    return *found;
}
struct TeamInput {
    const TeamTechnologies *owned=nullptr;
    const std::vector<std::int64_t> *share=nullptr,*met=nullptr;
    bool alive=false, has_tech=false;
};
std::vector<TeamInput> teams(const LiveSessionState &state,
                             const content::TechnologyRules &technologies,
                             const content::InfoTypeCatalog &types,
                             std::int64_t technology) {
    require(state.world.teams && state.world.teams->layout==bnw &&
            state.world.teams->dimensions.teams==64 &&
            state.world.teams->dimensions.civ_teams==63 &&
            state.world.teams->teams.size()==64 &&
            state.technologies.size()==64 &&
            technologies.profile==content::TechnologyProfile::bnw &&
            technologies.all_succeeded() &&
            types.report().all_succeeded() &&
            !types.report().hash_collisions && !types.report().name_overwrites,
            "BNW tech-share team or active technology profile is incomplete");
    const auto* actual=technology<0?nullptr:technologies.find(static_cast<std::uint64_t>(technology));
    const auto* mapped=actual?types.find(actual->base.type):nullptr;
    require(mapped&&mapped->table=="Technologies"&&mapped->id==technology&&
            mapped->hash==content::saved_info_type_hash(mapped->type),
            "BNW actual tech-share type identity is ambiguous");
    std::vector<TeamInput> result;
    result.reserve(64);
    for(std::int64_t id=0;id<64;++id) {
        const auto &raw=state.world.teams->teams[static_cast<std::size_t>(id)];
        const auto &owned=state.technologies[static_cast<std::size_t>(id)];
        require(raw.index==static_cast<std::uint64_t>(id) &&
                named(raw.fields,"id").value==id &&
                owned.team()==id &&
                owned.profile()==content::TechnologyProfile::bnw &&
                owned.slots()==technologies.collection.slots &&
                owned.remap().all_mapped(),
                "BNW tech-share team identity or research remap changed");
        const auto alive=named(raw.fields,"alive_count").value;
        require(alive>=0 && alive<=64,
                "BNW tech-share team alive count is invalid");
        const auto &share=named(raw.arrays,"tech_share");
        const auto &met=named(raw.arrays,"has_met");
        require(share.columns==1 && met.columns==1 &&
                share.values.size()==64 && met.values.size()==64,
                "BNW tech-share count or contact span is unavailable");
        for(const auto count:share.values)
            require(count>=0 && count<=INT32_MAX,
                    "BNW tech-share threshold count is invalid");
        for(const auto flag:met.values)
            require(flag==0 || flag==1,
                    "BNW tech-share met flag is invalid");
        const auto &hashes=named(raw.arrays,"tech_hashes");
        const auto &flags=named(raw.arrays,"has_tech");
        require(hashes.columns==1 && flags.columns==1 &&
                hashes.values.size()==flags.values.size(),
                "BNW tech-share saved technology arrays disagree");
        std::size_t matches=0;
        std::int64_t raw_owned=0;
        for(std::size_t i=0;i<hashes.values.size();++i)
            if(hashes.values[i]==static_cast<std::int64_t>(mapped->hash)) {
                ++matches;
                raw_owned=flags.values[i];
            }
        require(matches==1 && (raw_owned==0 || raw_owned==1) &&
                (raw_owned!=0)==owned.has_tech(technology),
                "BNW tech-share raw and owned Agriculture flags disagree");
        result.push_back({&owned,&share.values,&met.values,
                          alive>0,raw_owned!=0});
    }
    return result;
}
std::optional<std::int64_t> first_owed_city(
    const LiveSessionState &state,std::int64_t team) {
    require(state.world.all_players && state.world.all_players->layout==bnw &&
            state.world.all_players->players.size()==64 &&
            state.setup.players.size()==64,
            "BNW major-player culture building roster is incomplete");
    // MAX_MAJOR_CIVS is 22. Source only searches cities of major players on
    // the newly researched team, in ascending player slot order.
    for(std::int64_t player=0;player<22;++player) {
        const auto roster=state.setup.players.find(player);
        require(roster!=state.setup.players.end(),
                "BNW major-player culture building slot is missing");
        if(roster->second.team!=team) continue;
        const auto &saved=state.world.all_players->players[static_cast<std::size_t>(player)];
        require(saved.index==static_cast<std::uint64_t>(player) &&
                saved.saved_player_id==player && saved.cities.layout==bnw,
                "BNW major-player culture building identity changed");
        for(const auto &city:saved.cities.cities) {
            require(city.production.layout==bnw,
                    "BNW owed culture building city profile changed");
            const auto &before=named(city.production.blocks,"before_strategy");
            const auto owed=named(before.fields,"m_bOwedCultureBuilding").value;
            require(owed==0 || owed==1,
                    "BNW owed culture building flag is invalid");
            if(owed) return player;
        }
    }
    return std::nullopt;
}

std::vector<BnwTeamTechShareCheck> share_checks(const std::vector<TeamInput>& roster,std::int64_t team){
 std::vector<BnwTeamTechShareCheck> result;
 for(std::int64_t recipient=0;recipient<64;++recipient){
  if(recipient==team||!roster[static_cast<std::size_t>(recipient)].alive)continue;
  const auto& own=roster[static_cast<std::size_t>(recipient)];
  BnwTeamTechShareCheck check;check.recipient=recipient;check.already_owned=own.has_tech;
  if(!own.has_tech){
   for(std::size_t i=0;i<64;++i)if((*own.share)[i]>0){check.best_share=static_cast<std::int64_t>(i+1);break;}
   if(check.best_share>0){
    for(std::size_t other=0;other<63;++other)if(roster[other].alive&&roster[other].has_tech&&(*own.met)[other]==1)++check.met_owner_count;
    check.would_grant=check.met_owner_count>=check.best_share;
   }
  }
  result.push_back(check);if(check.would_grant)break;
 }
 return result;
}
} // namespace

BnwTeamTechShareCultureResult run_bnw_team_tech_share_culture(
    LiveSessionState &state,const BnwTeamTechLuaAttempt &lua,
    const content::TechnologyRules &technologies,
    const content::RulesDatabase &database,
    const content::InfoTypeCatalog &types,
    std::int64_t team,std::int64_t technology) {
    BnwTeamTechShareCultureResult result;
    const auto prefix=plan_bnw_team_tech_after_lua(
        state,lua,technologies,database,types,team,technology);
    if(prefix.first_pending!=BnwTeamTechAfterLuaStep::best_route ||
       prefix.proven_no_effect.size()!=4) {
        result.blocker=prefix.blocker.empty() ?
            "BNW tech-share post-Lua prefix is incomplete" : prefix.blocker;
        return result;
    }
    const auto *player_loop=state.setup.extensions.find<
        BnwTeamTechPlayerResearchMarker>();
    if(!player_loop || player_loop->team!=team ||
       player_loop->technology!=technology ||
       state.setup.extensions.find<BnwTeamTechShareCultureMarker>()) {
        result.blocker="BNW tech-share requires one completed player research loop";
        return result;
    }
    // Expansion2 CvTeam::DoBarbarianTech invokes setHasTech with both flags
    // false. No first-unit/free-tech or announcement code is reachable.
    result.proven_no_effect.push_back(
        BnwTeamTechShareCultureStep::first_to_research);
    result.proven_no_effect.push_back(
        BnwTeamTechShareCultureStep::announce);
    try {
        const auto roster=teams(state,technologies,types,technology);
        result.checked_teams=share_checks(roster,team);
        if(!result.checked_teams.empty()&&result.checked_teams.back().would_grant){
            result.first_pending=BnwTeamTechShareCultureStep::tech_share;
            result.blocker="CvTeam::updateTechShare would recursively grant Agriculture";
            return result;
        }
        result.proven_no_effect.push_back(BnwTeamTechShareCultureStep::tech_share);
        if(const auto player=first_owed_city(state,team)) {
            result.first_pending=
                BnwTeamTechShareCultureStep::owed_culture_buildings;
            result.blocker="CvCity::ChooseFreeCultureBuilding and SetNumFreeBuilding are required for major player "+
                std::to_string(*player);
            return result;
        }
        result.proven_no_effect.push_back(
            BnwTeamTechShareCultureStep::owed_culture_buildings);
        // Marker allocation precedes publication and is the only detached
        // write in the selected no-effect interval.
        state.setup.extensions.emplace<BnwTeamTechShareCultureMarker>(
            BnwTeamTechShareCultureMarker{team,technology,result.checked_teams});
        result.first_pending=BnwTeamTechShareCultureStep::later_set_has_tech;
        result.applied=true;
    } catch(const std::exception &error) {
        result.blocker=error.what();
    }
    return result;
}
} // namespace civ5::runtime

namespace civ5::runtime::detail {
std::vector<BnwTeamTechShareCheck> quote_bnw_team_tech_share_checks(
 const LiveSessionState& state,const content::TechnologyRules& technologies,
 const content::InfoTypeCatalog& types,std::int64_t team,std::int64_t technology){
 const auto roster=teams(state,technologies,types,technology);
 // Full raw/owned values, including progress/noTrade/count/lastTech, before
 // interpreting another team's live HasTech; requested flag alone is not proof.
 for(std::size_t id=0;id<roster.size();++id){
  const auto decoded=restore_team_technologies(state.world.teams->teams[id],technologies,types);
  const auto& owned=*roster[id].owned;
  require(decoded.remap().all_mapped()&&!decoded.remap().duplicates&&decoded.profile()==owned.profile()&&
   decoded.team()==owned.team()&&decoded.slots()==owned.slots()&&decoded.last_tech()==owned.last_tech()&&
   decoded.nondefault_values()==owned.nondefault_values()&&decoded.profile_extensions().values.empty()&&owned.profile_extensions().values.empty(),
   "BNW genuine tech-share full team values differ");
 }
 return share_checks(roster,team);
}
std::optional<std::int64_t> quote_bnw_team_tech_first_owed_culture_player(const LiveSessionState& state,std::int64_t team){
 return first_owed_city(state,team);
}
}

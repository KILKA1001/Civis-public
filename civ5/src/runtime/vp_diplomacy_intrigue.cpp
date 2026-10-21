#include "civ5/runtime/vp_diplomacy_intrigue.hpp"
#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
const saves::PlayerDiplomacyField &field(const saves::PlayerDiplomacyObservation &source,
                                         std::string_view name,std::uint64_t rows,
                                         std::uint64_t columns) {
    const saves::PlayerDiplomacyField *found=nullptr;
    for(const auto &item:source.fields)if(item.name&&std::string_view(item.name)==name) {
        if(found)throw std::runtime_error("Duplicate VP intrigue diplomacy field: "+std::string(name));
        found=&item;
    }
    if(!found||found->rows!=rows||found->columns!=columns||
       columns!=0&&rows>std::numeric_limits<std::size_t>::max()/columns||
       found->values.size()!=rows*columns)
        throw std::runtime_error("Unresolved VP intrigue diplomacy field: "+std::string(name));
    return *found;
}
std::uint32_t hash32(std::uint32_t input) {
    const auto state=input*747796405u+2891336453u;
    const auto word=((state>>((state>>28u)+4u))^state)*277803737u;
    return (word>>22u)^word;
}
std::uint32_t mix_raw(std::uint32_t seed,std::uint32_t value) {
    return seed^(value+0x9e3779b9u+(seed<<6u)+(seed>>2u));
}
std::uint32_t hashed(std::uint64_t value) {
    auto result=hash32(static_cast<std::uint32_t>(value));
    if(value>UINT32_MAX)result=mix_raw(result,hash32(static_cast<std::uint32_t>(value>>32u)));
    return result;
}
std::size_t intrigue_roll(std::size_t limit,std::uint64_t map_seed,std::int64_t turn,
                          std::int64_t x,std::int64_t y,std::size_t index) {
    if(!limit||turn<0||x<0||y<0)
        throw std::runtime_error("Invalid VP intrigue seeded major-player shuffle");
    // CvPlot::GetPseudoRandomSeed(): CvSeeder(x*17+y*23); then .mix(ui).
    const auto x64=static_cast<std::uint64_t>(x),y64=static_cast<std::uint64_t>(y);
    const auto plot_raw=x64*17u+y64*23u;
    auto plot=hashed(plot_raw);
    auto extra=mix_raw(plot,hashed(index));
    auto game=mix_raw(static_cast<std::uint32_t>(map_seed),hashed(static_cast<std::uint64_t>(turn)));
    if(map_seed>UINT32_MAX)game=mix_raw(game,hashed(map_seed>>32u));
    game=mix_raw(game,extra);
    if(limit<=UINT32_MAX)return game%limit;
    const auto extended=(std::uint64_t{game}<<32u)|hash32(game);
    return static_cast<std::size_t>(extended%limit);
}
const ResearchTeamState &team(const PlayerResearchContext &diplomacy,std::int64_t id) {
    const auto it=diplomacy.teams.find(id);
    if(it==diplomacy.teams.end())throw std::runtime_error("Unresolved VP intrigue team");
    return it->second;
}
bool vassal_pair(const PlayerResearchContext &diplomacy,std::int64_t first,
                 std::int64_t second) {
    const auto &a=team(diplomacy,first),&b=team(diplomacy,second);
    return a.master==second||b.master==first;
}
const VpDiplomacyIntrigueRelation &relation(const VpDiplomacyIntriguePlayer &player,
                                             std::int64_t other) {
    const auto it=player.relations.find(other);
    if(it==player.relations.end())throw std::runtime_error("Unresolved VP intrigue approach");
    return it->second;
}
std::int64_t surface_approach(const VpDiplomacyIntrigueRelation &state,bool at_war) {
    // Release-5.4.2 CvDiplomacyAI::GetSurfaceApproach, CivApproachTypes 0..6.
    if(at_war)return 0;
    if(state.dof_accepted_turn!=-1)return 6;
    if(state.approach==0)return state.cached_surface==-1?5:state.cached_surface;
    if(state.approach==2)return 6;
    return state.approach;
}
}
VpDiplomacyIntrigueProfileState &ensure_vp_diplomacy_intrigue_profile(ResearchContext &world) {
    if(world.profile!=vp)throw std::runtime_error("VP intrigue diplomacy requires VP GameCore profile");
    if(auto *profile=world.extensions.find<VpDiplomacyIntrigueProfileState>())return *profile;
    return world.extensions.emplace<VpDiplomacyIntrigueProfileState>();
}
const VpDiplomacyIntrigueProfileState *find_vp_diplomacy_intrigue_profile(const ResearchContext &world) {
    return world.profile==vp?world.extensions.find<VpDiplomacyIntrigueProfileState>():nullptr;
}
void restore_vp_diplomacy_intrigue_profile(const saves::SavedSessionSnapshot &snapshot,
                                           ResearchContext &world) {
    if(world.profile!=vp||!snapshot.all_players||
       snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus)
        throw std::runtime_error("VP intrigue diplomacy requires VP saved players");
    const auto major=snapshot.all_players->context.diplomacy.major;
    if(major>snapshot.all_players->players.size()||major>std::numeric_limits<std::size_t>::max())
        throw std::runtime_error("Incomplete VP intrigue major-player domain");
    std::map<std::uint64_t,const saves::SavedPlayerObservation*> by_slot;
    for(const auto &source:snapshot.all_players->players)
        if(!by_slot.emplace(source.index,&source).second||
           !world.players.contains(source.saved_player_id))
            throw std::runtime_error("Invalid VP intrigue saved player roster");
    VpDiplomacyIntrigueProfileState imported;
    for(std::uint64_t slot=0;slot<major;++slot) {
        const auto it=by_slot.find(slot);
        if(it==by_slot.end())throw std::runtime_error("Missing VP intrigue major-player slot");
        imported.major_order.push_back(it->second->saved_player_id);
    }
    for(std::uint64_t slot=0;slot<major;++slot) {
        const auto &saved=*by_slot.at(slot);
        const auto &source=saved.diplomacy;
        if(source.layout!=saves::GameStateLayout::vp542_corpus||
           source.dimensions.major!=major||
           source.dimensions.civ_players<major)
            throw std::runtime_error("VP intrigue diplomacy layout mismatch");
        const auto &approach=field(source,"m_aeCivApproach",1,source.dimensions.civ_players);
        const auto &cached=field(source,"m_aeCachedSurfaceApproach",1,major);
        const auto &dof=field(source,"m_aiDoFAcceptedTurn",1,major);
        const auto &coop=field(source,"m_aaeCoopWarState",major,major);
        VpDiplomacyIntriguePlayer player;
        for(std::size_t other=0;other<imported.major_order.size();++other) {
            const auto id=imported.major_order[other];
            const auto a=approach.values[other],c=cached.values[other];
            if(a<-1||a>6||c<-1||c>6)
                throw std::runtime_error("Invalid VP intrigue approach value");
            player.relations.emplace(id,VpDiplomacyIntrigueRelation{a,c,dof.values[other]});
            for(std::size_t third=0;third<imported.major_order.size();++third)
                player.coop_wars.emplace(std::pair{id,imported.major_order[third]},
                    coop.values[other*imported.major_order.size()+third]);
        }
        if(!imported.players.emplace(saved.saved_player_id,std::move(player)).second)
            throw std::runtime_error("Duplicate VP intrigue diplomacy player");
    }
    ensure_vp_diplomacy_intrigue_profile(world)=std::move(imported);
}
VpDiplomaticIntrigueScan vp_scan_diplomatic_intrigue(
    const ResearchContext &setup,const PlayerResearchContext &diplomacy,
    const content::CustomModOptions &options,std::int64_t spy_owner,
    std::int64_t target_owner,std::int64_t city_x,std::int64_t city_y) {
    const auto *profile=find_vp_diplomacy_intrigue_profile(setup);
    if(!profile||diplomacy.profile!=vp||!options.all_succeeded()||
       !setup.players.contains(spy_owner)||!setup.players.contains(target_owner)||
       !profile->players.contains(target_owner))
        throw std::runtime_error("Unresolved VP intrigue diplomacy profile");
    std::vector<std::int64_t> majors;
    for(const auto id:profile->major_order) {
        const auto it=setup.players.find(id);
        if(it==setup.players.end())throw std::runtime_error("Stale VP intrigue major-player roster");
        if(it->second.alive)majors.push_back(id);
    }
    for(std::size_t i=0;i<majors.size();++i)
        std::swap(majors[i],majors[intrigue_roll(majors.size(),setup.map_random_seed,
            setup.game_turn,city_x,city_y,i)]);
    const auto &target=profile->players.at(target_owner);
    const auto target_team=setup.players.at(target_owner).team;
    const auto spy_team=setup.players.at(spy_owner).team;
    VpDiplomaticIntrigueScan result;
    result.target_human_diplomacy=setup.is_human(setup.players.at(target_owner))&&
        !options.enabled("AI_CONTROL_DIPLOMACY");
    result.active_major_count=majors.size();
    if(result.target_human_diplomacy)
        for(const auto other:majors) {
            if(other==target_owner)continue;
            const auto other_team=setup.players.at(other).team;
            if(vassal_pair(diplomacy,other_team,target_team)||
               team(diplomacy,target_team).at_war.contains(other_team)||
               !team(diplomacy,spy_team).met.contains(other_team))continue;
            for(const auto third:majors) {
                if(third==target_owner||setup.players.at(third).team==other_team)continue;
                ++result.coop_pairs_examined;
                const auto it=target.coop_wars.find(std::pair{third,other});
                if(it==target.coop_wars.end())throw std::runtime_error("Unresolved VP coop-war state");
                if(it->second>=2) {
                    result.candidate=VpDiplomaticIntrigueCandidate{
                        VpDiplomaticIntrigueKind::coop_war,other,third,other};
                    return result;
                }
            }
        }
    if(!result.target_human_diplomacy)
        for(const auto other:majors) {
            if(other==target_owner)continue;
            const auto other_team=setup.players.at(other).team;
            if(vassal_pair(diplomacy,other_team,target_team))continue;
            ++result.deception_players_examined;
            const auto &state=relation(target,other);
            if(state.approach!=0&&state.approach!=2)continue;
            const auto at_war=team(diplomacy,target_team).at_war.contains(other_team);
            const auto surface=surface_approach(state,at_war);
            if(surface!=6&&surface!=5&&surface!=3)continue;
            if(at_war)continue;
            const auto reported=team(diplomacy,spy_team).met.contains(other_team)?other:-1;
            result.candidate=VpDiplomaticIntrigueCandidate{
                VpDiplomaticIntrigueKind::deception,other,-1,reported};
            break;
        }
    return result;
}
} // namespace civ5::runtime

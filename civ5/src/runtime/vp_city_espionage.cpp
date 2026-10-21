#include "civ5/runtime/vp_city_espionage.hpp"
#include "civ5/runtime/wide_arithmetic.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto vp=content::TechnologyProfile::vp542;
constexpr std::array<std::string_view,19> source_fields={
    "m_aiSpyAssignment","m_aiAmount","m_aiMaxAmount","m_aiNextPassiveBonus",
    "m_aiRate","m_aiGoal","m_aiLastProgress","m_aiLastPotential",
    "m_aiLastBasePotential","m_aiResult","m_aiNumTimesCityRobbed",
    "m_aiPendingEventsForPlayer","m_aiDiplomatTradeBonus",
    "m_abDiplomatReceiveIntrigues","m_abDiplomatRevealTrueApproaches",
    "m_aiDiplomatVoteTradePercent","m_aiSciencePassivePerTurn",
    "m_abRevealCityScreen","m_aiVisionBonus"};
template<class T> const T &named(const std::vector<T> &rows,std::string_view name) {
    const T *found=nullptr;
    for(const auto &row:rows)if(row.name&&std::string_view(row.name)==name) {
        if(found)throw std::runtime_error("Duplicate VP city espionage field: "+std::string(name));
        found=&row;
    }
    if(!found)throw std::runtime_error("Missing VP city espionage field: "+std::string(name));
    return *found;
}
std::int64_t scalar(const saves::CityCoreObservation &core,std::string_view name) {
    const auto &values=named(core.fields,name).values;
    if(values.size()!=1)throw std::runtime_error("VP city espionage identity width mismatch");
    return values.front();
}
}
std::int64_t VpCityEspionageState::value(std::string_view field,std::int64_t player) const {
    const auto it=fields.find(field);
    if(it==fields.end()||!it->second.contains(player))
        throw std::runtime_error("VP city espionage player field is unavailable: "+std::string(field));
    return it->second.at(player);
}
void VpCityEspionageState::set(std::string_view field,std::int64_t player,std::int64_t value) {
    const auto it=fields.find(field);
    if(it==fields.end()||!it->second.contains(player))
        throw std::runtime_error("VP city espionage player field is unavailable: "+std::string(field));
    it->second.at(player)=value;
}
VpCityEspionageState &ensure_vp_city_espionage(CityYieldContext &world,
    std::int64_t owner,std::int64_t city) {
    if(world.profile!=vp)throw std::runtime_error("VP city espionage requires VP GameCore profile");
    auto &live=world.players.at(owner).cities.at(city);
    if(auto *state=live.extensions.find<VpCityEspionageState>())return *state;
    return live.extensions.emplace<VpCityEspionageState>();
}
const VpCityEspionageState *find_vp_city_espionage(const CityYieldContext &world,
    std::int64_t owner,std::int64_t city) {
    if(world.profile!=vp)return nullptr;
    const auto player=world.players.find(owner);
    if(player==world.players.end())return nullptr;
    const auto live=player->second.cities.find(city);
    return live==player->second.cities.end()?nullptr:live->second.extensions.find<VpCityEspionageState>();
}
void restore_vp_city_espionage(const saves::SavedSessionSnapshot &snapshot,CityYieldContext &world) {
    if(world.profile!=vp||!snapshot.all_players||snapshot.all_players->layout!=saves::GameStateLayout::vp542_corpus)
        throw std::runtime_error("VP city espionage import requires VP cities");
    std::map<std::pair<std::int64_t,std::int64_t>,VpCityEspionageState> imported;
    for(const auto &saved_player:snapshot.all_players->players) {
        const auto owner=saved_player.saved_player_id;
        const auto live_player=world.players.find(owner);
        if(live_player==world.players.end())throw std::runtime_error("VP city espionage player is missing");
        for(const auto &saved_city:saved_player.cities.cities) {
            if(saved_city.tail.layout!=saves::GameStateLayout::vp542_corpus||
               saved_city.core.layout!=saves::GameStateLayout::vp542_corpus)
                throw std::runtime_error("VP city espionage layout mismatch");
            const auto id=scalar(saved_city.core,"m_iID");
            const auto live=live_player->second.cities.find(id);
            if(live==live_player->second.cities.end()||live->second.owner!=owner||
               scalar(saved_city.core,"m_eOwner")!=owner||
               live->second.x!=scalar(saved_city.core,"m_iX")||
               live->second.y!=scalar(saved_city.core,"m_iY"))
                throw std::runtime_error("VP city espionage is not bound to the live city");
            VpCityEspionageState state;
            state.security_modifier=scalar(saved_city.core,"m_iSpySecurityModifier");
            state.security_per_x_pop=scalar(saved_city.core,"m_iSpySecurityModifierPerXPop");
            state.previous_spy_missions=scalar(saved_city.core,"m_iNumPreviousSpyMissions");
            state.happiness_delta=scalar(saved_city.core,"m_iHappinessDelta");
            const auto &record=named(saved_city.tail.blocks,"espionage");
            for(const auto name:source_fields) {
                const auto &values=named(record.children,name).values;
                auto &players=state.fields[std::string(name)];
                for(std::size_t i=0;i<values.size();++i) {
                    if(i>static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max()))
                        throw std::runtime_error("VP city espionage wire index exceeds host identity range");
                    players.emplace(static_cast<std::int64_t>(i),values[i]);
                }
            }
            const auto legacy_next=state.fields.find("m_aiNextPassiveBonus");
            for(const auto &[player,value]:legacy_next->second) {
                VpCityEspionageState::NextPassiveBonus next;
                if(value==-1)next.initialized=false;
                else if(value==std::numeric_limits<std::int32_t>::max())next.initialized=true;
                else if(value>=0) {
                    next.initialized=true;
                    next.threshold=value;
                } else throw std::runtime_error("Invalid saved VP passive bonus threshold");
                state.next_passive_bonus.emplace(player,next);
            }
            state.fields.erase(legacy_next);
            if(!imported.emplace(std::pair{owner,id},std::move(state)).second)
                throw std::runtime_error("Duplicate VP city espionage record");
        }
    }
    std::size_t live_count=0;
    for(const auto &[id,player]:world.players)live_count+=player.cities.size();
    if(imported.size()!=live_count)throw std::runtime_error("Incomplete VP city espionage roster");
    for(auto &[key,state]:imported)
        ensure_vp_city_espionage(world,key.first,key.second)=std::move(state);
}
std::optional<VpSpyCityBinding> vp_spy_city_binding(const CityYieldContext &world,
    std::int64_t spy_owner,std::size_t spy_index,std::int64_t x,std::int64_t y) {
    if(world.profile!=vp)throw std::runtime_error("VP spy city binding requires VP GameCore profile");
    for(const auto &[owner,player]:world.players)
        for(const auto &[id,city]:player.cities)if(city.x==x&&city.y==y) {
            const auto *state=find_vp_city_espionage(world,owner,id);
            if(!state)throw std::runtime_error("Missing VP city espionage activity");
            const auto assignment=state->value("m_aiSpyAssignment",spy_owner);
            if(spy_index>static_cast<std::size_t>(std::numeric_limits<std::int64_t>::max())||
               assignment!=static_cast<std::int64_t>(spy_index))
                throw std::runtime_error("VP spy assignment disagrees with target city");
            return VpSpyCityBinding{owner,id,x,y,assignment,
                state->value("m_aiAmount",spy_owner),state->value("m_aiMaxAmount",spy_owner),
                state->value("m_aiRate",spy_owner),state->value("m_aiGoal",spy_owner)};
        }
    return {};
}
VpCitySpyProcessResult process_vp_city_spy_activity(VpCityEspionageState &city,
    std::int64_t player,std::int64_t rate) {
    const auto before=city.value("m_aiAmount",player);
    const auto max_before=city.value("m_aiMaxAmount",player);
    if(before<0||max_before<0||rate<0)
        throw std::runtime_error("VP city spy activity has not been initialized");
    const auto after=wide::add(before,rate);
    const auto max_after=std::max(max_before,after);
    city.set("m_aiRate",player,rate);
    city.set("m_aiLastProgress",player,rate);
    city.set("m_aiAmount",player,after);
    city.set("m_aiMaxAmount",player,max_after);
    return {before,after,max_before,max_after,rate};
}
} // namespace civ5::runtime

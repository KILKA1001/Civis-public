#include "civ5/runtime/bnw_city_flavor_recipients.hpp"
#include "civ5/runtime/live_session.hpp"
#include <set>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
static_assert(std::is_nothrow_move_assignable_v<BnwCityFlavorRecipients>);

void require(bool ok,const char *message) {
    if(!ok) throw std::runtime_error(message);
}
std::int64_t city_scalar(const saves::CityCoreObservation &core,
                         std::string_view name) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(field.name && std::string_view(field.name)==name) {
            require(!found,"Duplicate BNW city flavor identity field");
            found=&field;
        }
    require(found && found->values.size()==1,
            "BNW city flavor identity field unavailable");
    return found->values.front();
}
bool valid_block(const char *name,std::string_view expected,
                 std::uint64_t start,std::uint64_t end) {
    return name && std::string_view(name)==expected && start<end;
}
BnwCityFlavorRecipients collect(const LiveSessionState &state) {
    require(state.world.game_state.layout==bnw && state.world.all_players &&
            state.world.all_players->layout==bnw &&
            state.setup.profile==content::TechnologyProfile::bnw &&
            state.cities.profile==content::TechnologyProfile::bnw,
            "BNW city flavor registration requires owned BNW players and cities");
    BnwCityFlavorRecipients result;
    std::set<std::int64_t> player_ids;
    std::set<BnwCityHandle> city_ids;
    for(const auto &saved:state.world.all_players->players) {
        const auto owner=saved.saved_player_id;
        require(owner>=0 && player_ids.insert(owner).second &&
                saved.cities.layout==bnw && saved.knowledge.layout==bnw,
                "BNW city flavor player identity/profile mismatch");
        const auto live=state.cities.players.find(owner);
        require(live!=state.cities.players.end() &&
                live->second.cities.size()==saved.cities.cities.size(),
                "BNW city flavor live roster differs from decoded owner");
        bool manager=false;
        for(const auto &block:saved.knowledge.blocks)
            if(block.name && std::string_view(block.name)=="flavor_manager") {
                require(!manager && valid_block(block.name,"flavor_manager",
                                                block.start_offset,block.end_offset),
                        "BNW player flavor manager block is invalid");
                manager=true;
            }
        require(manager,"BNW player flavor manager not restored");
        auto &recipients=result.by_owner[owner];
        for(const auto &city:saved.cities.cities) {
            require(city.core.layout==bnw && city.management.layout==bnw,
                    "BNW city flavor strategy profile mismatch");
            const auto id=city_scalar(city.core,"m_iID");
            const auto city_owner=city_scalar(city.core,"m_eOwner");
            require(id>=0 && city_owner==owner,
                    "BNW city flavor owner or city ID is invalid");
            bool strategy=false;
            for(const auto &block:city.management.blocks)
                if(block.name && std::string_view(block.name)=="strategy") {
                    require(!strategy && valid_block(block.name,"strategy",
                                                     block.start_offset,block.end_offset),
                            "BNW city flavor strategy block is invalid");
                    strategy=true;
                }
            require(strategy,"BNW city flavor strategy not restored");
            const auto found=live->second.cities.find(id);
            require(found!=live->second.cities.end() &&
                    found->second.owner==owner && found->second.id==id &&
                    found->second.source_core==std::optional{std::pair{
                        city.core.start_offset,city.core.end_offset}},
                    "BNW city flavor live city differs from decoded strategy owner");
            const BnwCityHandle handle{owner,id};
            require(city_ids.insert(handle).second,
                    "Duplicate BNW city flavor recipient");
            recipients.push_back(handle);
        }
    }
    for(const auto &[owner,live]:state.cities.players)
        if(!live.cities.empty())
            require(player_ids.contains(owner),
                    "BNW city flavor live owner absent from decoded world");
    result.current=true;
    return result;
}
} // namespace

void rebuild_bnw_city_flavor_recipients(LiveSessionState &state) {
    state.city_flavor_recipients.current=false;
    auto replacement=collect(state);
    state.city_flavor_recipients=std::move(replacement);
}

const std::vector<BnwCityHandle> &bnw_city_flavor_recipients(
    const LiveSessionState &state,std::int64_t owner) {
    const auto &cache=state.city_flavor_recipients;
    require(cache.current,"BNW city flavor registrations are not current");
    const auto source=collect(state);
    require(cache.by_owner==source.by_owner,
            "BNW city flavor registrations became stale");
    const auto found=cache.by_owner.find(owner);
    require(found!=cache.by_owner.end(),
            "BNW city flavor owner unavailable");
    return found->second;
}

} // namespace civ5::runtime

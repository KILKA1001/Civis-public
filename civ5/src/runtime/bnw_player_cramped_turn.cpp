#include "civ5/runtime/bnw_player_cramped_turn.hpp"
#include "civ5/runtime/bnw_live_plot_impassability.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include "civ5/content/rules_database.hpp"
#include <limits>
#include <map>
#include <string_view>
#include <variant>

namespace civ5::runtime {
namespace {
using B = BnwCrampedTurnBlocker;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool condition,B blocker,const char *message) {
    if(!condition) throw BnwCrampedTurnError(blocker,message);
}
template<class Field> const Field &unique_field(const auto &fields,
                                                 std::string_view name,B blocker) {
    const Field *found=nullptr;
    for(const auto &field:fields)
        if(field.name && std::string_view(field.name)==name) {
            require(!found,blocker,"duplicate BNW cramped field");
            found=&field;
        }
    require(found && found->values.size()==1,blocker,
            "BNW cramped field unavailable or malformed");
    return *found;
}
std::int32_t source_int(std::int64_t value,B blocker) {
    require(value>=std::numeric_limits<std::int32_t>::min() &&
            value<=std::numeric_limits<std::int32_t>::max(),blocker,
            "BNW cramped value exceeds source int");
    return static_cast<std::int32_t>(value);
}
std::int32_t define(const content::GlobalDefineCache &cache,std::string_view name) {
    const auto *entry=cache.find(name);
    require(entry && entry->lookup.found && entry->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(entry->lookup.value),
            B::selected_rules,"selected BNW cramped define unavailable");
    return source_int(std::get<std::int64_t>(entry->lookup.value),B::selected_rules);
}
std::int64_t half_floor(std::int64_t value) {
    return value/2-(value<0 && value%2!=0 ? 1 : 0);
}
struct City {std::int32_t x=0,y=0;};
std::vector<City> cities(const LiveSessionState &state,
                          const saves::SavedPlayerObservation &saved,
                          std::int64_t player_id,const saves::SavedMapProjection &map) {
    require(state.cities.profile==content::TechnologyProfile::bnw &&
            saved.cities.layout==bnw,B::city_roster,
            "BNW cramped city profile unavailable");
    const auto live=state.cities.players.find(player_id);
    require(live!=state.cities.players.end() &&
            live->second.cities.size()==saved.cities.cities.size(),B::city_roster,
            "BNW cramped live city roster differs from decoded owner");
    std::vector<City> result;
    result.reserve(saved.cities.cities.size());
    for(const auto &source:saved.cities.cities) {
        require(source.core.layout==bnw,B::city_roster,
                "BNW cramped city core profile differs");
        const auto &id_field=unique_field<saves::CityCoreField>(
            source.core.fields,"m_iID",B::city_roster);
        const auto &owner_field=unique_field<saves::CityCoreField>(
            source.core.fields,"m_eOwner",B::city_roster);
        const auto &x_field=unique_field<saves::CityCoreField>(
            source.core.fields,"m_iX",B::city_roster);
        const auto &y_field=unique_field<saves::CityCoreField>(
            source.core.fields,"m_iY",B::city_roster);
        const auto id=id_field.values[0];
        const auto x=source_int(x_field.values[0],B::city_roster);
        const auto y=source_int(y_field.values[0],B::city_roster);
        require(id>=0 && owner_field.values[0]==player_id && x>=0 && y>=0 &&
                static_cast<std::uint64_t>(x)<map.grid.width &&
                static_cast<std::uint64_t>(y)<map.grid.height,B::city_roster,
                "BNW cramped city location or owner invalid");
        const auto found=live->second.cities.find(id);
        require(found!=live->second.cities.end() &&
                found->second.owner==player_id && found->second.id==id &&
                found->second.x==x && found->second.y==y &&
                found->second.source_core==std::optional{std::pair{
                    source.core.start_offset,source.core.end_offset}},B::city_roster,
                "BNW cramped live city differs from decoded source");
        const auto index=map.grid.index(x,y);
        const auto &cell=map.cells.at(static_cast<std::size_t>(index));
        require(cell.plot_city && cell.plot_city->owner==player_id &&
                cell.plot_city->id==id && cell.is_city,B::city_roster,
                "BNW cramped city plot link or post-read city flag missing");
        result.push_back({x,y});
    }
    return result;
}
} // namespace

BnwCrampedTurnStep apply_bnw_player_cramped_turn(
    LiveSessionState &state,std::int64_t player_id,
    const content::RulesDatabase &rules,const SessionCheckpointIdentity &identity) {
    require(state.setup.profile==content::TechnologyProfile::bnw &&
            state.world.game_state.layout==bnw &&
            state.world.map_header.layout==bnw &&
            state.world.all_players && state.world.all_players->layout==bnw,
            B::profile,"BNW cramped turn requires decoded Expansion2 profile");
    require(!identity.rules_sha256.empty() &&
            identify_bytes(rules.gameplay_fingerprint_data()).sha256==identity.rules_sha256,
            B::selected_rules,"BNW cramped turn selected rules identity differs");
    const auto defines=rules.cache_global_defines(content::GlobalDefineProfile::bnw);
    const auto range=define(defines,"CRAMPED_RANGE_FROM_CITY");
    const auto threshold=define(defines,"CRAMPED_USABLE_PLOT_PERCENT");
    // Finite host budget and source int protection. Selected BNW uses range 5.
    require(range>=0 && range<=64,B::source_integer_range,
            "BNW cramped range exceeds verified bounded traversal");
    const auto setup=state.setup.players.find(player_id);
    require(setup!=state.setup.players.end() && setup->second.alive,
            B::player,"BNW cramped turn requires an alive restored player");
    saves::SavedPlayerObservation *saved=nullptr;
    for(auto &candidate:state.world.all_players->players)
        if(candidate.saved_player_id==player_id) {
            require(!saved,B::player,"duplicate saved BNW player ID");
            saved=&candidate;
        }
    require(saved && saved->core.layout==bnw,B::player,
            "BNW cramped turn saved player missing");
    const auto &id=unique_field<saves::PlayerCoreField>(
        saved->core.fields,"m_eID",B::player);
    const auto &alive=unique_field<saves::PlayerCoreField>(
        saved->core.fields,"m_bAlive",B::player);
    const auto &flag=unique_field<saves::PlayerCoreField>(
        saved->core.fields,"m_bCramped",B::player);
    require(id.encoding==saves::PlayerCoreEncoding::i32 &&
            id.values[0]==player_id &&
            alive.encoding==saves::PlayerCoreEncoding::boolean &&
            alive.values[0]==1 &&
            flag.encoding==saves::PlayerCoreEncoding::boolean &&
            (flag.values[0]==0 || flag.values[0]==1),B::player,
            "BNW cramped saved player identity or flag invalid");
    require(state.world.map_projection.has_value(),B::map,
            "BNW cramped owned map unavailable");
    const auto &map=*state.world.map_projection;
    require(map.grid.width==state.world.map_header.grid.width &&
            map.grid.height==state.world.map_header.grid.height &&
            map.wrap_x==state.world.map_header.wrap_x &&
            map.wrap_y==state.world.map_header.wrap_y &&
            map.cells.size()==map.grid.size() &&
            map.grid.width<=std::numeric_limits<std::int32_t>::max()-64 &&
            map.grid.height<=std::numeric_limits<std::int32_t>::max()-64,
            B::map,"BNW cramped owned map identity or source bounds differ");
    MapGeometry geometry{map.grid,map.wrap_x,map.wrap_y,bnw};
    try {geometry.validate();} catch(const std::exception &) {
        throw BnwCrampedTurnError(B::map,"BNW cramped map geometry invalid");
    }
    const auto source_cities=cities(state,*saved,player_id,map);
    const PlotPassability *pass=nullptr;
    try {pass=&bnw_live_plot_impassability(state);} catch(const std::exception &) {
        throw BnwCrampedTurnError(B::plot_impassability,
                                  "BNW cramped post-load plot impassability is stale");
    }
    require(state.plot_impassability.rules_sha256==identity.rules_sha256 &&
            pass->plots.size()==map.cells.size(),B::plot_impassability,
            "BNW cramped plot passability rules identity differs");

    std::int64_t total=0,usable=0;
    for(const auto &city:source_cities)
        for(std::int32_t dx=-range;dx<=range;++dx)
            for(std::int32_t dy=-range;dy<=range;++dy) {
                const auto abs_x=dx>=0?dx:-dx,abs_y=dy>=0?dy:-dy;
                const auto hex_range=(dx>=0)==(dy>=0)?abs_x+abs_y:
                    std::max(abs_x,abs_y);
                if(hex_range>range) continue;
                const auto plot_y=static_cast<std::int64_t>(city.y)+dy;
                const auto hex_x=static_cast<std::int64_t>(city.x)-
                    half_floor(city.y)+dx;
                const auto plot_x=hex_x+half_floor(plot_y);
                const auto index=geometry.plot(plot_x,plot_y);
                if(!index) continue;
                const auto &cell=map.cells.at(static_cast<std::size_t>(*index));
                const auto xy=map.grid.coordinates(*index);
                require(cell.index==static_cast<std::uint64_t>(*index) &&
                        cell.x==xy[0] && cell.y==xy[1] &&
                        cell.owner && cell.plot_type && *cell.owner>=-1 &&
                        *cell.owner<static_cast<std::int64_t>(
                            state.world.all_players->context.player_count) &&
                        *cell.plot_type>=0 && *cell.plot_type<=3,B::map,
                        "BNW cramped nearby plot state incomplete");
                // CvPlot::isOwned is owner != NO_PLAYER (-1). Source plot enum:
                // PLOT_MOUNTAIN=0 and PLOT_OCEAN=3.
                if(*cell.owner==-1 || *cell.owner!=player_id) {
                    ++total;
                    if(*cell.owner==-1 &&
                       !pass->plots[static_cast<std::size_t>(*index)].impassable &&
                       *cell.plot_type!=0 && *cell.plot_type!=3)
                        ++usable;
                }
                require(total<=std::numeric_limits<std::int32_t>::max() &&
                        usable<=std::numeric_limits<std::int32_t>::max()/100,
                        B::source_integer_range,
                        "BNW cramped source int arithmetic would overflow");
            }
    const bool before=flag.values[0]!=0;
    const bool wrote=total>0;
    const bool after=wrote ? (100*usable/total<=threshold) : before;
    if(wrote)
        for(auto &candidate:saved->core.fields)
            if(candidate.name && std::string_view(candidate.name)=="m_bCramped") {
                candidate.values[0]=after?1:0;
                break;
            }
    return {player_id,before,after,wrote,static_cast<std::int32_t>(total),
            static_cast<std::int32_t>(usable)};
}
} // namespace civ5::runtime

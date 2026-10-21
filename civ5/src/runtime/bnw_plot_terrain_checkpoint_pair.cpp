#include "civ5/runtime/bnw_unit_move_pending.hpp"
#include "civ5/runtime/bnw_plot_terrain_checkpoint_pair.hpp"
#include "civ5/runtime/bnw_plot_yield_refresh.hpp"
#include "civ5/runtime/bnw_city_strategy_yields.hpp"
#include "civ5/runtime/bnw_unowned_plot_yield.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/content/saved_terrain.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <map>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
using Key=std::pair<std::int64_t,std::int64_t>;
using Six=std::array<std::int64_t,6>;
void need(bool valid,const char* why){if(!valid)throw std::runtime_error(why);}
auto& named(auto& rows,std::string_view wanted){
    using Row=std::remove_reference_t<decltype(rows.front())>;
    Row* result=nullptr;
    for(auto& row:rows)if(row.name&&wanted==row.name){
        need(!result,"Plot/terrain pair duplicate source field");result=&row;
    }
    need(result,"Plot/terrain pair source field absent");return *result;
}
std::int64_t scalar(const auto& rows,std::string_view wanted){
    const auto& field=named(rows,wanted);
    need(field.encoding==saves::CityCoreEncoding::i32&&field.values.size()==1&&
         field.values.front()>=INT32_MIN&&field.values.front()<=INT32_MAX,
         "Plot/terrain pair city scalar shape differs");
    return field.values.front();
}
auto& terrain(auto& city){
    auto& field=named(city.core.fields,"m_aiBaseYieldRateFromTerrain");
    need(field.encoding==saves::CityCoreEncoding::vector_i32&&field.wire_count==6&&
         field.values.size()==6&&field.bytes.empty()&&field.row_keys.empty()&&
         field.row_counts.empty()&&field.hashes.empty(),
         "Plot/terrain pair native six-terrain vector unavailable");
    for(const auto value:field.values)
        need(value>=INT32_MIN&&value<=INT32_MAX,"Plot/terrain pair terrain exceeds i32");
    return field;
}
Six cached(const saves::SavedSessionSnapshot& world,std::size_t index){
    need(world.map_plots&&world.map_projection&&world.map_plots->dimensions.yields==6,
         "Plot/terrain pair native map yields absent");
    const auto& raw=named(world.map_plots->plots.at(index).arrays,"yields");
    const auto& cell=world.map_projection->cells.at(index);
    need(raw.columns==1&&raw.values.size()==6&&cell.cached_yields&&
         cell.cached_yields->size()==6,"Plot/terrain pair six-cache shape differs");
    Six result{};
    for(std::size_t y=0;y<6;++y){
        const auto found=cell.cached_yields->find(static_cast<std::int64_t>(y));
        need(raw.values[y]>=INT16_MIN&&raw.values[y]<=INT16_MAX&&
             found!=cell.cached_yields->end()&&found->second==raw.values[y],
             "Plot/terrain pair raw/projected cache differs");result[y]=raw.values[y];
    }
    return result;
}
std::map<Key,const saves::SavedCityObservation*> city_roster(
    const saves::SavedSessionSnapshot& world){
    need(world.game_state.layout==bnw&&world.all_players&&
         world.all_players->layout==bnw,"Plot/terrain pair source city roster absent");
    std::map<Key,const saves::SavedCityObservation*> result;
    for(const auto& player:world.all_players->players)for(const auto& city:player.cities.cities){
        const Key key{player.saved_player_id,scalar(city.core.fields,"m_iID")};
        need(city.core.layout==bnw&&key.second>=0&&
             scalar(city.core.fields,"m_eOwner")==key.first&&result.emplace(key,&city).second,
             "Plot/terrain pair source city identity differs");(void)terrain(city);
    }
    return result;
}
void owned_mirrors(const LiveSessionState& state){
    need(state.setup.profile==content::TechnologyProfile::bnw&&
         state.cities.profile==content::TechnologyProfile::bnw&&state.cities.yield_slots==6,
         "Plot/terrain pair owned BNW six-yield context absent");
    const auto roster=city_roster(state.world);std::size_t owned_count=0;
    for(const auto& [owner,player]:state.cities.players){
        for(const auto& [id,city]:player.cities){
            ++owned_count;const auto source=roster.find({owner,id});
            need(source!=roster.end()&&city.owner==owner&&city.id==id&&
                 city.source_core==std::optional{std::pair{source->second->core.start_offset,
                                                         source->second->core.end_offset}}&&
                 city.x==scalar(source->second->core.fields,"m_iX")&&
                 city.y==scalar(source->second->core.fields,"m_iY"),
                 "Plot/terrain pair raw/owned city binding differs");
            const auto& values=terrain(*source->second).values;
            for(std::int64_t y=0;y<6;++y){
                const auto found=city.yields.find(y);
                need((found==city.yields.end()?0:found->second.terrain)==values.at(static_cast<std::size_t>(y)),
                     "Plot/terrain pair raw/owned terrain mirror differs");
            }
        }
    }
    need(owned_count==roster.size(),"Plot/terrain pair owned city roster incomplete");
}
// SDK plotCityXY inverse, identical single-step wrap and odd-row conversion
// to the actual refresh leaf. Repeated small-map offsets do not become a new
// choice of citizen slot: native ascending slot order selects the first match.
int city_slot(const saves::SavedMapProjection& map,std::int64_t cx,std::int64_t cy,
              std::int64_t px,std::int64_t py){
    const auto wrap=[](std::int64_t n,std::uint64_t size,bool enabled){
        need(size>0&&size<=INT32_MAX,"Plot/terrain pair native grid dimension differs");
        const auto s=static_cast<std::int64_t>(size);
        if(enabled){if(n>s/2)return n-s;if(n<-s/2)return n+s;}return n;
    };
    const auto half=[](std::int64_t n){return n/2-(n<0&&n%2?1:0);};
    const auto wx=wrap(px-cx,map.grid.width,map.wrap_x),dy=wrap(py-cy,map.grid.height,map.wrap_y);
    const auto dx=wrap((cx+wx-half(cy+dy))-(cx-half(cy)),map.grid.width,map.wrap_x);
    constexpr int xs[]={0,0,1,1,0,-1,-1,0,1,2,2,2,1,0,-1,-2,-2,-2,-1,0,1,2,3,3,3,3,2,1,0,-1,-2,-3,-3,-3,-3,-2,-1};
    constexpr int ys[]={0,1,0,-1,-1,0,1,2,1,0,-1,-2,-2,-2,-1,0,1,2,2,3,2,1,0,-1,-2,-3,-3,-3,-3,-2,-1,0,1,2,3,3,3};
    for(int i=0;i<37;++i)if(xs[i]==dx&&ys[i]==dy)return i;return -1;
}
struct Change {std::int64_t plot;std::uint8_t writes;bool worked;};
std::vector<Change> verify_relation(const saves::SavedSessionSnapshot& source,
                                  const LiveSessionState& current){
    owned_mirrors(current);const auto before=city_roster(source),after=city_roster(current.world);
    need(source.map_projection&&source.map_plots&&current.world.map_projection&&
         current.world.map_plots&&source.map_plots->plots.size()==current.world.map_plots->plots.size(),
         "Plot/terrain pair source map shape differs");
    std::map<Key,Six> deltas;std::vector<Change> changes;
    for(std::size_t index=0;index<source.map_plots->plots.size();++index){
        const auto base=cached(source,index),live=cached(current.world,index);if(base==live)continue;
        need(index<=static_cast<std::size_t>(INT64_MAX),"Plot/terrain pair plot exceeds signed index");
        const auto& raw=source.map_plots->plots[index];const auto& cell=source.map_projection->cells.at(index);
        const Key key{named(raw.fields,"working_city_owner").value,named(raw.fields,"working_city_id").value};
        need(cell.yield_city&&cell.yield_city->owner==key.first&&cell.yield_city->id==key.second,
             "Plot/terrain pair working-city raw/projected handle differs");
        need(key.first>=INT32_MIN&&key.first<=INT32_MAX&&key.second>=INT32_MIN&&key.second<=INT32_MAX,
             "Plot/terrain pair working-city handle exceeds native int");
        Key canonical=key;
        bool worked=false;const auto* actual=resolve_bnw_current_city(current,
            static_cast<std::int32_t>(key.first),static_cast<std::int32_t>(key.second));
        if(actual){
            canonical={scalar(actual->core.fields,"m_eOwner"),scalar(actual->core.fields,"m_iID")};
            need(canonical.first==key.first,"Plot/terrain pair resolved city owner differs");
            const auto found=before.find(canonical);need(found!=before.end(),"Plot/terrain pair working city absent from source");
            const auto& city=*found->second;const auto cx=scalar(city.core.fields,"m_iX"),cy=scalar(city.core.fields,"m_iY");
            need(cx>=0&&cy>=0&&static_cast<std::uint64_t>(cx)<source.map_header.grid.width&&
                 static_cast<std::uint64_t>(cy)<source.map_header.grid.height&&
                 cell.x<=INT32_MAX&&cell.y<=INT32_MAX,
                 "Plot/terrain pair working coordinates exceed native map");
            const auto& block=named(city.management.blocks,"citizens");
            const auto& flags=named(block.arrays,"m_pabWorkingPlot");
            need(city.management.layout==bnw&&flags.columns==1&&flags.values.size()==37&&
                 std::all_of(flags.values.begin(),flags.values.end(),[](auto n){return n==0||n==1;}),
                 "Plot/terrain pair source working flags unavailable");
            const auto slot=city_slot(*source.map_projection,cx,cy,static_cast<std::int64_t>(cell.x),static_cast<std::int64_t>(cell.y));
            worked=slot>=0&&flags.values.at(static_cast<std::size_t>(slot))!=0;
        }
        std::uint8_t writes=0;
        for(std::size_t y=0;y<6;++y)if(base[y]!=live[y]){
            need(live[y]==0&&bnw_plot_early_zero_yield(current,static_cast<std::int64_t>(index),static_cast<std::int64_t>(y)).has_value(),
                 "Plot/terrain pair changed cache has no implemented early-zero source");
            ++writes;if(worked){auto& delta=deltas[canonical][y];
                need((base[y]>=0?delta>=INT64_MIN+base[y]:delta<=INT64_MAX+base[y]),
                     "Plot/terrain pair delta accumulation overflow");delta-=base[y];}
        }
        changes.push_back({static_cast<std::int64_t>(index),writes,worked});
    }
    need(before.size()==after.size(),"Plot/terrain pair city roster changed");
    for(const auto& [key,city]:before){
        const auto found=after.find(key);need(found!=after.end(),"Plot/terrain pair city removed");
        const auto& base=terrain(*city).values;const auto& live=terrain(*found->second).values;
        const auto found_delta=deltas.find(key);
        for(std::size_t y=0;y<6;++y){
            const auto delta=found_delta==deltas.end()?0:found_delta->second[y];
            need(base[y]>=0?delta<=INT64_MAX-base[y]:delta>=INT64_MIN-base[y],
                 "Plot/terrain pair expected terrain overflow");
            const auto expected=base[y]+delta;
            need(expected>=INT32_MIN&&expected<=INT32_MAX&&live[y]==expected,
                 "Plot/terrain pair terrain change disagrees with actual worked plot caches");
        }
    }
    return changes;
}
BnwPlotTerrainCheckpointPair sections(const LiveSessionState& state,
    const saves::SavedSessionSnapshot& source,const SessionCheckpointIdentity& identity,
    const saves::SavedTerrainFeatureResolver& features){
    (void)verify_relation(source,state);
    auto normalized=state.world;
    for(std::size_t i=0;i<source.map_plots->plots.size();++i){
        named(normalized.map_plots->plots.at(i).arrays,"yields").values=
            named(source.map_plots->plots.at(i).arrays,"yields").values;
        normalized.map_projection->cells.at(i).cached_yields=source.map_projection->cells.at(i).cached_yields;
    }
    const auto originals=city_roster(source);
    for(auto& player:normalized.all_players->players)for(auto& city:player.cities.cities){
        const Key key{player.saved_player_id,scalar(city.core.fields,"m_iID")};
        const auto old=originals.find(key);need(old!=originals.end(),"Plot/terrain pair city identity changed");
        terrain(city).values=terrain(*old->second).values;
    }
    const auto unchanged_core=make_bnw_city_core_checkpoint(normalized,source,identity,true);
    need(unchanged_core.bytes.at(8)==2,"Plot/terrain pair has unsupported other city-core mutation");
    const auto unchanged_map=make_bnw_map_plot_checkpoint(normalized,source,identity,features);
    need(unchanged_map.bytes==make_bnw_map_plot_checkpoint(source,source,identity,features).bytes,
         "Plot/terrain pair has unsupported other map/player mutation");
    return {make_bnw_map_plot_checkpoint(state.world,source,identity,features),
            make_bnw_city_core_checkpoint(state.world,source,identity,true)};
}
void source_identity(const LiveSession& session,const SessionCheckpointIdentity& identity){
    need(identify_bytes(session.source().source.bytes).sha256==identity.payload_sha256,
         "Plot/terrain pair source payload identity differs");
}
}
struct BnwPlotTerrainRulesSnapshot::Impl {
    std::thread::id thread=std::this_thread::get_id();
    std::string rules_sha256;
    saves::SavedTerrainFeatureResolver features;
};
BnwPlotTerrainRulesSnapshot::BnwPlotTerrainRulesSnapshot(std::shared_ptr<const Impl> owned)
    :impl_(std::move(owned)){}
BnwPlotTerrainRulesSnapshot BnwPlotTerrainRulesSnapshot::from_rules(const content::RulesDatabase& rules){
    auto result=std::make_shared<Impl>();result->rules_sha256=rules.gameplay_fingerprint_sha256();
    const auto types=rules.cache_info_types();
    result->features=content::saved_terrain_features(rules,types).resolve;
    need(rules.gameplay_fingerprint_sha256()==result->rules_sha256,
         "Plot/terrain pair rules changed while creating immutable snapshot");
    return BnwPlotTerrainRulesSnapshot(std::move(result));
}
BnwPlotTerrainCheckpointPair make_bnw_plot_terrain_checkpoint_pair(
    const LiveSession& session,const SessionCheckpointIdentity& identity,
    const BnwPlotTerrainRulesSnapshot& rules){
    const auto rules_owner=rules.impl_;
    need(rules_owner&&rules_owner->thread==std::this_thread::get_id(),
         "Plot/terrain pair rules owner absent or foreign thread before live borrow");
    need(session.plot_yield_refresh_thread_==std::this_thread::get_id(),
         "Plot/terrain pair session history foreign thread");
    // This component format cannot persist the food native once journal.
    need(!session.food_growth_registry_,
         "Plot/terrain pair cannot export food-growth once history");
    need(!session.unit_move_registry_||!session.unit_move_registry_->has_history(),
         "Plot/terrain pair cannot export incomplete unit-move once history");
    // Registered native37 city calls own once history before their first plot.
    // This byte-component format cannot persist it, even with no cache writes.
    need(!session.city_yield_refresh_registry_,
         "Plot/terrain pair cannot export city-yield once history");
    // Even three already-false reset calls own private once history. This
    // component pair cannot encode it; reject BEFORE any world/query/codec.
    need(session.production_turn_pending_.empty(),
         "Plot/terrain pair cannot export production-turn once history");
    need(session.city_entry_pending_.empty(),
         "Plot/terrain pair cannot export city-entry once history");
    // Inspect ALL actual once slots before changed-plot discovery. An unchanged
    // or zero-result normal operation must not hide beside early-zero writes.
    for(const auto& [plot,pending]:session.plot_yield_refresh_pending_){
        (void)plot;need(static_cast<bool>(pending),"Plot/terrain pair unissued session history");
        pending->require_early_zero_checkpoint_mode();
    }
    const auto state=session.read_state_lease();
    need(static_cast<bool>(state),"Plot/terrain pair live allocation unavailable");
    need(rules_owner&&rules_owner->rules_sha256==identity.rules_sha256,
         "Plot/terrain pair immutable rules snapshot identity differs");
    const auto& features=rules_owner->features;
    source_identity(session,identity);const auto generation=session.state_generation();
    const auto changes=verify_relation(session.source().snapshot,*state);
    need(!changes.empty(),"Plot/terrain pair has no actual refresh cache writes");
    std::vector<std::shared_ptr<const BnwPlotYieldRefreshPending>> owners;
    for(const auto& change:changes){
        auto pending=session.bnw_plot_yield_refresh_pending(change.plot);const auto p=pending->progress();
        need(p.plot==change.plot&&p.operation!=0&&p.plot_writes==change.writes&&
             p.terrain_writes==(change.worked?change.writes:0)&&!p.effect_started,
             "Plot/terrain pair SAME issued source prefix differs or effect in flight");
        owners.push_back(std::move(pending));
    }
    auto result=sections(*state,session.source().snapshot,identity,features);
    for(const auto& owner:owners)owner->require_current();
    need(session.read_state_lease().get()==state.get()&&session.state_generation()==generation,
         "Plot/terrain pair actual allocation changed during capture");return result;
}
BnwPlotTerrainRestoredComponents restore_bnw_plot_terrain_checkpoint_pair(
    const BnwPlotTerrainCheckpointPair& pair,const LiveSession& baseline,
    const SessionCheckpointIdentity& identity,const BnwPlotTerrainRulesSnapshot& rules){
    const auto rules_owner=rules.impl_;
    need(rules_owner&&rules_owner->thread==std::this_thread::get_id(),
         "Plot/terrain pair rules owner absent or foreign thread before live borrow");
    need(baseline.plot_yield_refresh_thread_==std::this_thread::get_id(),
         "Plot/terrain pair baseline history foreign thread");
    // This component format cannot persist the food native once journal.
    need(!baseline.food_growth_registry_,
         "Plot/terrain pair cannot restore over food-growth once history");
    need(!baseline.unit_move_registry_||!baseline.unit_move_registry_->has_history(),
         "Plot/terrain pair cannot restore over unit-move once history");
    need(baseline.production_turn_pending_.empty(),
         "Plot/terrain pair cannot restore over production-turn once history");
    need(!baseline.city_yield_refresh_registry_,
         "Plot/terrain pair cannot restore over city-yield once history");
    const auto state=baseline.read_state_lease();
    need(static_cast<bool>(state),"Plot/terrain pair baseline allocation unavailable");
    need(rules_owner&&rules_owner->rules_sha256==identity.rules_sha256,
         "Plot/terrain pair immutable rules snapshot identity differs");
    const auto& features=rules_owner->features;
    source_identity(baseline,identity);const auto generation=baseline.state_generation();
    owned_mirrors(*state);const auto& source=baseline.source().snapshot;
    need(make_bnw_city_core_checkpoint(state->world,source,identity,true).bytes.at(8)==2&&
         make_bnw_map_plot_checkpoint(state->world,source,identity,features).bytes==
         make_bnw_map_plot_checkpoint(source,source,identity,features).bytes,
         "Plot/terrain pair destination is not source baseline");
    auto detached=*state;
    restore_bnw_city_core_checkpoint(pair.city_core,detached.world,source,identity);
    restore_bnw_map_plot_checkpoint(pair.map,detached.world,identity,features);
    for(const auto& [key,city]:city_roster(detached.world)){
        auto& owned=detached.cities.players.at(key.first).cities.at(key.second);
        const auto& values=terrain(*city).values;
        for(std::int64_t y=0;y<6;++y){const auto value=values.at(static_cast<std::size_t>(y));
            const auto found=owned.yields.find(y);
            if(found!=owned.yields.end())found->second.terrain=value;
            else if(value){CityYieldComponents components;components.terrain=value;
                owned.yields.emplace(y,components);}
        }
    }
    const auto canonical=sections(detached,source,identity,features);
    need(canonical.map.bytes==pair.map.bytes&&canonical.city_core.bytes==pair.city_core.bytes,
         "Plot/terrain pair byte components are not canonical matching facts");
    need(baseline.read_state_lease().get()==state.get()&&baseline.state_generation()==generation,
         "Plot/terrain pair baseline allocation changed during detached restore");
    return {std::move(detached.world),std::move(detached.cities)};
}
}

#include "civ5/runtime/bnw_unit_sight_plan.hpp"
#include "civ5/runtime/bnw_native_unit_allocator_core.hpp"
#include "civ5/ui/selected_bnw_preview_host.hpp"
#include <set>
#include "civ5/runtime/bnw_live_plot_visibility.hpp"
#include "civ5/runtime/file_identity.hpp"
#include "civ5/runtime/live_session.hpp"
#include "civ5/runtime/map_geometry.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <map>
#include <optional>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
std::int64_t i32(std::int64_t value) {
    require(value>=INT32_MIN && value<=INT32_MAX,
            "BNW sight SDK i32 arithmetic overflow");
    return value;
}
std::int64_t field(const saves::PlotObservation &plot,std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &item:plot.fields)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate BNW sight plot field");found=&item;
        }
    require(found,"Missing BNW sight plot field");return found->value;
}
std::int64_t invisible_count(const saves::PlotObservation &plot,
                             std::int64_t team,std::uint64_t teams) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:plot.arrays)
        if(name(array.name)=="invisible_visibility") {
            require(!found,"Duplicate BNW invisible visibility array");found=&array;
        }
    require(found && found->columns==1 && found->values.size()==teams &&
            team>=0 && static_cast<std::uint64_t>(team)<teams,
            "BNW invisible visibility wire dimensions are unsupported");
    const auto value=found->values[static_cast<std::size_t>(team)];
    require(value>=0 && value<=INT16_MAX,
            "BNW invisible visibility source is outside safe i16 range");
    return value;
}
std::int64_t team_field(const saves::TeamObservation &team,std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &item:team.fields)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate BNW sight team field");found=&item;
        }
    require(found,"Missing BNW sight team field");return found->value;
}
std::int64_t core(const saves::SavedUnitObservation &unit,std::string_view wanted,
                  saves::UnitCoreEncoding encoding) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &item:unit.core.fields)
        if(name(item.name)==wanted) {
            require(!found,"Duplicate BNW sight unit field");found=&item;
        }
    require(found && found->encoding==encoding && found->values.size()==1,
            "Missing BNW sight unit field");return found->values.front();
}
std::int64_t define(const content::GlobalDefineCache &cache,std::string_view wanted) {
    require(cache.profile()==content::GlobalDefineProfile::bnw,
            "BNW sight requires BNW global define cache");
    const auto *entry=cache.find(wanted);
    require(entry && entry->lookup.found && entry->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(entry->lookup.value),
            "Missing BNW sight global define");
    return i32(std::get<std::int64_t>(entry->lookup.value));
}
struct Terrain {std::int64_t from=0,through=0;};
struct Rules {
    std::map<std::int64_t,std::pair<std::uint32_t,std::int64_t>> units;
    std::map<std::int64_t,Terrain> terrains;
    std::map<std::uint32_t,std::int64_t> features;
    std::int64_t recon=0,mountain_from=0,mountain_through=0;
    std::int64_t hills_from=0,hills_through=0,water_from=0,water_through=0;
};
Rules load_rules(const content::RulesDatabase &db,const content::InfoTypeCatalog &types,
                 const content::GlobalDefineCache &defines) {
    require(types.report().all_succeeded(),"BNW sight InfoType catalog has errors");
    Rules rules;
    const auto verify=[&](const std::string &type,std::string_view table,
                          std::int64_t id) {
        const auto *entry=types.find(type);
        require(entry && entry->table==table && entry->id==id &&
                entry->hash==content::saved_info_type_hash(type) &&
                types.find_hash(entry->hash) &&
                types.find_hash(entry->hash)->type==type,
                "BNW sight rule ID/hash/catalog mismatch");
        return entry->hash;
    };
    const auto units=db.query_rows({"SELECT ID,Type,BaseSightRange FROM Units",{}},
        [&](const auto &row) {
            const auto type=row.text("Type");
            require(type && row.has_column("ID") && row.has_column("BaseSightRange"),
                    "Incomplete BNW sight unit rule");
            const auto id=i32(row.integer("ID"));
            const auto hash=verify(std::string(*type),"Units",id);
            require(rules.units.emplace(id,std::pair{hash,i32(row.integer("BaseSightRange"))}).second,
                    "Duplicate BNW sight unit ID");
        });
    require(units.all_succeeded() && units.rows==rules.units.size(),
            "BNW sight unit rules unavailable");
    const auto terrains=db.query_rows({"SELECT ID,Type,SeeFrom,SeeThrough FROM Terrains",{}},
        [&](const auto &row) {
            const auto type=row.text("Type");
            require(type && row.has_column("ID") && row.has_column("SeeFrom") &&
                    row.has_column("SeeThrough"),"Incomplete BNW sight terrain rule");
            const auto id=i32(row.integer("ID"));
            (void)verify(std::string(*type),"Terrains",id);
            require(rules.terrains.emplace(id,Terrain{i32(row.integer("SeeFrom")),
                        i32(row.integer("SeeThrough"))}).second,
                    "Duplicate BNW sight terrain ID");
        });
    require(terrains.all_succeeded() && terrains.rows==rules.terrains.size(),
            "BNW sight terrain rules unavailable");
    const auto features=db.query_rows({"SELECT ID,Type,SeeThrough FROM Features",{}},
        [&](const auto &row) {
            const auto type=row.text("Type");
            require(type && row.has_column("ID") && row.has_column("SeeThrough"),
                    "Incomplete BNW sight feature rule");
            const auto id=i32(row.integer("ID"));
            const auto hash=verify(std::string(*type),"Features",id);
            require(rules.features.emplace(hash,i32(row.integer("SeeThrough"))).second,
                    "Duplicate BNW sight feature hash");
        });
    require(features.all_succeeded() && features.rows==rules.features.size(),
            "BNW sight feature rules unavailable");
    rules.recon=define(defines,"RECON_VISIBILITY_RANGE");
    rules.mountain_from=define(defines,"MOUNTAIN_SEE_FROM_CHANGE");
    rules.mountain_through=define(defines,"MOUNTAIN_SEE_THROUGH_CHANGE");
    rules.hills_from=define(defines,"HILLS_SEE_FROM_CHANGE");
    rules.hills_through=define(defines,"HILLS_SEE_THROUGH_CHANGE");
    rules.water_from=define(defines,"SEAWATER_SEE_FROM_CHANGE");
    rules.water_through=define(defines,"SEAWATER_SEE_THROUGH_CHANGE");
    return rules;
}
struct Levels {std::int64_t from=0,through=0,through_outer=0;};
Levels levels(const saves::PlotObservation &plot,const Rules &rules,
              bool extra_water) {
    const auto terrain_id=field(plot,"terrain_type");
    const auto terrain=rules.terrains.find(terrain_id);
    require(terrain!=rules.terrains.end(),"BNW sight plot terrain ID is absent from active rules");
    const auto plot_type=field(plot,"plot_type");
    require(plot_type>=0 && plot_type<=3,"BNW sight plot type is invalid");
    const auto recon=field(plot,"recon_count");
    require(recon>=0,"BNW sight plot recon count is invalid");
    const auto raw_feature=field(plot,"feature_type_hash");
    require(raw_feature>=0 && raw_feature<=UINT32_MAX,
            "BNW sight plot feature hash is outside wire range");
    std::int64_t feature_change=0;
    if(raw_feature) {
        const auto feature=rules.features.find(static_cast<std::uint32_t>(raw_feature));
        require(feature!=rules.features.end(),
                "BNW sight plot feature hash is absent from active rules");
        feature_change=feature->second;
    }
    const auto from_base=recon>0?i32(rules.recon*2):terrain->second.from;
    auto from=from_base,through=terrain->second.through;
    if(plot_type==0) {
        from=i32(from+rules.mountain_from);
        through=i32(through+rules.mountain_through);
    } else if(plot_type==1) {
        from=i32(from+rules.hills_from);
        through=i32(through+rules.hills_through);
    } else if(plot_type==3) {
        from=i32(from+rules.water_from+(extra_water?1:0));
        through=i32(through+rules.water_through);
    }
    return {from,i32(through+feature_change),through};
}
std::int64_t floor_half(std::int64_t value) {
    return value/2-(value<0 && value%2!=0?1:0);
}
std::int64_t hex_distance(std::int64_t dx,std::int64_t dy) {
    return (dx>=0)==(dy>=0)?std::llabs(dx)+std::llabs(dy):
        std::max(std::llabs(dx),std::llabs(dy));
}
using SightCall=BnwSightCall;
std::vector<SightCall> scan_adjacent_sight(
    const saves::SavedSessionSnapshot &world,const MapGeometry &geometry,
    std::uint64_t center,std::int64_t range,const Rules &rules,bool extra_water) {
    constexpr std::array<std::int64_t,6> direction_x{0,1,1,0,-1,-1};
    constexpr std::array<std::int64_t,6> direction_y{1,0,-1,-1,0,1};
    constexpr std::int64_t half_blocked=0x01000000;
    const auto centre_xy=world.map_header.grid.coordinates(center);
    const auto cx=static_cast<std::int64_t>(centre_xy[0]);
    const auto cy=static_cast<std::int64_t>(centre_xy[1]);
    const auto extra=range+1;
    require(range>=0 && range<=8 && world.map_plots &&
            world.map_plots->plots.size()==world.map_header.grid.size(),
            "BNW sight range or map shape exceeds verified ring plan");
    std::vector<std::optional<std::int64_t>> scratch(world.map_header.grid.size());
    std::vector<std::optional<Levels>> heights(world.map_header.grid.size());
    std::vector<bool> visited(world.map_header.grid.size());
    std::vector<SightCall> calls;
    const auto height=[&](std::uint64_t index)->const Levels & {
        auto &saved=heights.at(index);
        if(!saved) saved=levels(world.map_plots->plots.at(index),rules,extra_water);
        return *saved;
    };
    const auto at_displacement=[&](std::int64_t dx,std::int64_t dy)
        ->std::optional<std::uint64_t> {
        if(hex_distance(dx,dy)>extra) return {};
        const auto y=cy+dy;
        const auto x=cx-floor_half(cy)+dx+floor_half(y);
        const auto plot=geometry.plot(x,y);
        return plot?std::optional(static_cast<std::uint64_t>(*plot)):std::nullopt;
    };
    const auto inward=[&](std::uint64_t current,std::int64_t direction,
                          std::int64_t ring)->std::optional<std::pair<std::int64_t,bool>> {
        const auto xy=world.map_header.grid.coordinates(current);
        const auto other=geometry.direction(xy[0],xy[1],direction);
        if(!other) return {};
        const auto neighbour=world.map_header.grid.coordinates(*other);
        if(geometry.distance(cx,cy,neighbour[0],neighbour[1])!=ring-1) return {};
        const auto value=scratch.at(static_cast<std::size_t>(*other));
        require(value.has_value(),"BNW sight inward scratch was not initialized");
        require(*value>=-half_blocked/2 && *value<half_blocked*2,
                "BNW sight scratch level is outside verified range");
        return std::pair{*value>=half_blocked?*value-half_blocked:*value,
                         *value>=half_blocked};
    };
    std::int64_t ring=0,next_direction=5,dx=0,dy=0,counter=0,ring_end=0;
    while(ring<=extra) {
        const auto current=at_displacement(dx,dy);
        if(current) {
            require(!visited.at(*current),
                    "BNW sight wrapped ring visits a plot twice");
            visited[*current]=true;
            if(ring==0) {
                calls.push_back({*current,ring});
                scratch[*current]=0;
            } else {
                std::int64_t first_dir=0,second_dir=0;
                switch(next_direction) {
                case 0:first_dir=1;second_dir=2;break; // NE: E, SE
                case 1:first_dir=3;second_dir=2;break; // E: SW, SE
                case 2:first_dir=3;second_dir=4;break; // SE: SW, W
                case 3:first_dir=4;second_dir=5;break; // SW: W, NW
                case 4:first_dir=5;second_dir=0;break; // W: NW, NE
                case 5:first_dir=1;second_dir=0;break; // NW: E, NE
                default:throw std::runtime_error("Invalid BNW sight ring direction");
                }
                const auto first=inward(*current,first_dir,ring);
                const auto second=inward(*current,second_dir,ring);
                const auto this_level=ring==extra?height(*current).through_outer:
                                                   height(*current).through;
                require(this_level>-half_blocked/2 && this_level<half_blocked/2,
                        "BNW sight terrain level collides with scratch flag");
                std::int64_t scratch_value=this_level;
                bool visible=false;
                const auto center_level=height(center).from;
                const auto choose=[&](std::int64_t low,std::int64_t high) {
                    scratch_value=std::max(high,this_level);
                    visible=low<this_level ||
                        (center_level>=low && ring<extra);
                };
                if(first && second && first->first!=second->first &&
                   !first->second && !second->second) {
                    const auto xy=world.map_header.grid.coordinates(*current);
                    const auto first_plot=geometry.direction(xy[0],xy[1],first_dir);
                    const auto second_plot=geometry.direction(xy[0],xy[1],second_dir);
                    require(first_plot && second_plot,"BNW sight inward geometry changed");
                    const auto a=world.map_header.grid.coordinates(*first_plot);
                    const auto b=world.map_header.grid.coordinates(*second_plot);
                    const double x0=static_cast<double>(cx)+(cy&1?0.5:0.0);
                    const double x1=static_cast<double>(xy[0])+(xy[1]&1?0.5:0.0);
                    const double line_a=static_cast<double>(xy[1])-cy;
                    const double line_b=x0-x1;
                    const double line_c=static_cast<double>(cy)*x1-
                                        static_cast<double>(xy[1])*x0;
                    const auto line_distance=[&](const std::array<std::uint64_t,2> &p) {
                        const double px=static_cast<double>(p[0])+(p[1]&1?0.5:0.0);
                        return std::fabs(line_a*px+line_b*p[1]+line_c);
                    };
                    const double first_distance=line_distance(a);
                    const double second_distance=line_distance(b);
                    if(first_distance-second_distance>0.05)
                        choose(second->first,second->first);
                    else if(second_distance-first_distance>0.05)
                        choose(first->first,first->first);
                    else {
                        const auto low=std::min(first->first,second->first);
                        const auto high=std::max(first->first,second->first);
                        scratch_value=high>this_level?
                            std::max(low,this_level)+half_blocked:
                            std::max(high,this_level);
                        visible=low<this_level ||
                            (center_level>=low && ring<extra);
                    }
                } else if(first && !first->second)
                    choose(first->first,first->first);
                else if(second && !second->second)
                    choose(second->first,second->first);
                else if(first)
                    choose(first->first,first->first);
                else if(second)
                    choose(second->first,second->first);
                scratch[*current]=scratch_value;
                if(visible) calls.push_back({*current,ring});
            }
        }
        std::int64_t next_dx=0,next_dy=0;
        if(counter>=ring_end) {
            next_dx=dx+direction_x[0];next_dy=dy+direction_y[0];
            ++ring;ring_end+=ring*6;next_direction=1;
        } else {
            next_dx=dx+direction_x[next_direction];
            next_dy=dy+direction_y[next_direction];
            if(hex_distance(next_dx,next_dy)>ring) {
                next_direction=(next_direction+1)%6;
                next_dx=dx+direction_x[next_direction];
                next_dy=dy+direction_y[next_direction];
            }
        }
        ++counter;dx=next_dx;dy=next_dy;
    }
    require(scratch[center].has_value(),"BNW sight center was not processed");
    return calls;
}
} // namespace

BnwUnitSightPlan plan_bnw_unit_sight(
    const LiveSessionState &state,const BnwSetXYRequest &request,
    const content::RulesDatabase &database,const content::InfoTypeCatalog &types,
    const content::GlobalDefineCache &defines,
    const SessionCheckpointIdentity &identity) {
    BnwUnitSightPlan result;
    try {
        const auto preflight=preflight_bnw_unit_sight(state,request);
        require(preflight.candidate,"BNW sight needs a verified simple unit candidate");
        require(preflight.see_invisible==-1,
                "BNW sight invisible-visibility counters need a separate plan");
        require(identity.rules_sha256.size()==64 &&
                identify_bytes(database.gameplay_fingerprint_data()).sha256==
                    identity.rules_sha256,
                "BNW sight active rules fingerprint differs from session identity");
        const auto rules=load_rules(database,types,defines);
        const auto unit_rule=rules.units.find(preflight.unit_type);
        require(unit_rule!=rules.units.end(),"BNW sight unit type ID is absent from active rules");
        const saves::SavedUnitObservation *selected=nullptr;
        for(const auto &player:state.world.all_players->players)
            for(const auto &unit:player.units.units)
                if(core(unit,"m_eOwner",saves::UnitCoreEncoding::i32)==request.owner &&
                   core(unit,"m_iID",saves::UnitCoreEncoding::i32)==request.unit_id)
                    selected=&unit;
        require(selected && core(*selected,"idUnitType",saves::UnitCoreEncoding::hash32)==
                unit_rule->second.first,
                "BNW sight saved unit hash differs from active type ID");
        const auto range=i32(unit_rule->second.second+preflight.extra_visibility);
        require(range>=0 && range<=8,"BNW sight unit range exceeds verified ring plan");
        const auto extra_water_count=team_field(
            state.world.teams->teams.at(static_cast<std::size_t>(preflight.team)),
            "extra_water_see_from");
        require(extra_water_count>=0 && extra_water_count<=INT32_MAX,
                "BNW sight team water-height modifier is invalid");
        const bool extra_water=extra_water_count>0;
        MapGeometry geometry{state.world.map_header.grid,
            state.world.map_header.wrap_x,state.world.map_header.wrap_y,
            saves::GameStateLayout::expansion2_version1};
        geometry.validate();
        const auto old_calls=scan_adjacent_sight(state.world,geometry,
            preflight.from_plot,range,rules,extra_water);
        const auto new_calls=scan_adjacent_sight(state.world,geometry,
            preflight.to_plot,range,rules,extra_water);
        const auto &cache=bnw_live_plot_visibility_cache(state);
        require(state.world.map_plots->dimensions.invisible_types==1 &&
                state.world.map_plots->dimensions.teams==80,
                "BNW sight invisible visibility source dimensions changed");
        std::map<std::uint64_t,BnwSightPlotDelta> changes;
        const auto entry=[&](std::uint64_t plot)->BnwSightPlotDelta & {
            auto [it,inserted]=changes.try_emplace(plot);
            if(inserted) {
                const auto before=cache.counts.at(static_cast<std::size_t>(
                    plot*cache.wire_slots+preflight.team));
                require(before>=0 && before<=INT16_MAX,
                        "BNW sight base visibility exceeds SDK short range");
                const auto invisible=invisible_count(
                    state.world.map_plots->plots.at(plot),preflight.team,80);
                it->second={plot,before,before,before,0,0,
                            invisible,invisible,invisible,0,0};
            }
            return it->second;
        };
        for(const auto call:old_calls) {
            auto &item=entry(call.plot);
            require(item.old_calls==0,"BNW sight old ring repeated a visibility count");
            item.old_calls=1;
            item.after_old=std::max<std::int64_t>(0,item.after_old-1);
            item.after_new=item.after_old;
            if(call.ring<2) {
                require(item.invisible_old_calls==0 && item.invisible_after_old>0,
                        "BNW sight old ring underflows invisible visibility");
                item.invisible_old_calls=1;
                --item.invisible_after_old;
                item.invisible_after_new=item.invisible_after_old;
            }
        }
        for(const auto call:new_calls) {
            auto &item=entry(call.plot);
            require(item.new_calls==0 && item.after_new<INT16_MAX,
                    "BNW sight new ring repeated or overflowed a visibility count");
            item.new_calls=1;
            item.after_new++;
            if(call.ring<2) {
                require(item.invisible_new_calls==0 &&
                        item.invisible_after_new<INT16_MAX,
                        "BNW sight new ring overflows invisible visibility");
                item.invisible_new_calls=1;
                ++item.invisible_after_new;
            }
        }
        result.calculated=true;
        result.team=preflight.team;result.visibility_range=range;
        result.new_call_order=new_calls;
        for(const auto &[plot,item]:changes) result.plots.push_back(item);
        result.blockers={BnwSightBlocker::reveal_meeting_and_exploration_effects,
                         BnwSightBlocker::invisible_visibility_effects,
                         BnwSightBlocker::ui_and_derived_cache_effects,
                         BnwSightBlocker::visibility_checkpoint};
        result.reason="BNW sight counters are read-only; reveal, team, AI and UI effects remain";
    } catch(const std::exception &error) {
        result.calculated=false;result.plots.clear();result.new_call_order.clear();
        result.blockers={BnwSightBlocker::unsupported_candidate};
        result.reason=error.what();
    }
    return result;
}
} // namespace civ5::runtime

namespace civ5::runtime {
namespace {
const saves::PlotArrayObservation&sight_array(const saves::PlotObservation&p,std::string_view key){
 const saves::PlotArrayObservation*result=nullptr;for(const auto&a:p.arrays)if(name(a.name)==key){require(!result,"Duplicate newborn sight array");result=&a;}
 require(result&&result->columns==1&&result->values.size()==80,"Newborn sight array dimensions differ");return *result;
}
saves::PlotArrayObservation&sight_mutable_array(saves::PlotObservation&p,std::string_view key){
 (void)sight_array(p,key);for(auto&a:p.arrays)if(name(a.name)==key)return a;
 throw std::runtime_error("Missing mutable newborn sight array");
}
struct SightStore {
 std::uint64_t plot=0;std::size_t cache_slot=0,team=0;
 std::vector<std::int64_t>*visibility=nullptr,*invisible=nullptr;
 const std::int64_t*visibility_data=nullptr,*invisible_data=nullptr;
 std::int64_t before=0,after=0,raw_before=0,invisible_before=0,invisible_after=0;
 bool based_on_unit=false;
};
template<class Emit>void sight_semantic(const LiveSessionState&s,std::int32_t team,Emit&&emit,const std::vector<SightStore>*after=nullptr){
 const auto&w=s.world;require(team>=0&&team<64&&w.game_state.layout==saves::GameStateLayout::expansion2_version1&&
  w.map_header.layout==saves::GameStateLayout::expansion2_version1&&
  w.map_plots&&w.map_plots->layout==saves::GameStateLayout::expansion2_version1&&w.map_projection&&w.teams,
  "Newborn sight actual map/team source absent");
 emit(w.map_header.grid.width);emit(w.map_header.grid.height);emit(w.map_header.wrap_x);emit(w.map_header.wrap_y);
 emit(w.map_projection->grid.width);emit(w.map_projection->grid.height);emit(w.map_projection->wrap_x);emit(w.map_projection->wrap_y);
 emit(w.map_projection->cells.size());emit(w.map_plots->plots.size());emit(w.map_plots->dimensions.teams);emit(w.map_plots->dimensions.invisible_types);
 const auto&cache=s.plot_visibility;emit(cache.current);emit(cache.grid.width);emit(cache.grid.height);emit(cache.team_slots);emit(cache.wire_slots);
 emit(cache.source_raw.size());emit(cache.counts.size());emit(w.teams->dimensions.teams);emit(w.teams->teams.size());
 require(cache.current&&cache.team_slots==64&&cache.wire_slots==80&&w.map_plots->dimensions.teams==80&&w.map_plots->dimensions.invisible_types==1&&
  w.map_plots->plots.size()<=SIZE_MAX/80&&
  w.map_plots->plots.size()==w.map_header.grid.size()&&w.map_projection->cells.size()==w.map_plots->plots.size()&&
  cache.grid.width==w.map_header.grid.width&&cache.grid.height==w.map_header.grid.height&&
  w.map_projection->grid.width==w.map_header.grid.width&&w.map_projection->grid.height==w.map_header.grid.height&&
  w.map_projection->wrap_x==w.map_header.wrap_x&&w.map_projection->wrap_y==w.map_header.wrap_y&&
  cache.counts.size()==w.map_plots->plots.size()*80&&cache.source_raw.size()==cache.counts.size()&&w.teams->teams.size()==64,
  "Newborn sight actual cache/source shape changed");
 emit(team_field(w.teams->teams.at(static_cast<std::size_t>(team)),"extra_water_see_from"));
 for(std::size_t i=0;i<w.map_plots->plots.size();++i){
  const auto&p=w.map_plots->plots[i];const auto&cell=w.map_projection->cells[i];
  const auto xy=w.map_header.grid.coordinates(i);require(p.index==i&&cell.index==i&&p.x==xy[0]&&cell.x==xy[0]&&p.y==xy[1]&&cell.y==xy[1],
   "Newborn sight map plot identity differs");
  emit(p.index);emit(p.x);emit(p.y);emit(cell.index);emit(cell.x);emit(cell.y);
  for(const auto*key:{"terrain_type","plot_type","recon_count","feature_type_hash"})emit(field(p,key));
  const auto&raw=sight_array(p,"visibility");const auto&invisible=sight_array(p,"invisible_visibility");
  const SightStore*replacement=nullptr;if(after)for(const auto&item:*after)if(item.plot==i)replacement=&item;
  const auto slot=i*80+static_cast<std::size_t>(team);const auto t=static_cast<std::size_t>(team);
  emit(replacement?replacement->after:raw.values[t]);emit(replacement?replacement->invisible_after:invisible.values[t]);
  emit(replacement?replacement->after:cache.source_raw[slot]);emit(replacement?replacement->after:cache.counts[slot]);
 }
}
void match_sight(const LiveSessionState&s,std::int32_t team,const std::vector<std::int64_t>&baseline){
 std::size_t i=0;sight_semantic(s,team,[&](auto value){require(i<baseline.size()&&baseline[i++]==static_cast<std::int64_t>(value),"Newborn sight source/cache changed in place");});
 require(i==baseline.size(),"Newborn sight semantic span changed");
}
}
struct BnwInitialUnitSightOwner::Impl {
 BnwNativeUnitAllocatorCore::InitialSightOperands args;
 std::weak_ptr<const BnwNativeUnitAllocatorCore> core;
 std::vector<std::int64_t> before,after;
 std::vector<SightStore> stores;
 const std::int64_t*cache_counts=nullptr,*cache_source=nullptr;
 bool applied=false;
};
BnwInitialUnitSightOwner::BnwInitialUnitSightOwner(std::unique_ptr<Impl>p)noexcept:impl_(std::move(p)){}
BnwInitialUnitSightOwner::~BnwInitialUnitSightOwner()=default;
std::shared_ptr<BnwInitialUnitSightOwner>BnwInitialUnitSightOwner::prepare(const BnwNativeUnitAllocatorCore&source){
 const auto canonical=source.retain_factory_owner();auto p=std::make_unique<Impl>();p->core=canonical;p->args=canonical->initial_sight_operands();
 auto&s=*p->args.visual.state;const auto&host=*p->args.visual.host;
 require(s.world.map_plots&&s.world.map_plots->plots.size()<=(SIZE_MAX-22)/14,"Newborn sight semantic storage span exceeds host");
 const auto seal_size=22+14*s.world.map_plots->plots.size();p->before.reserve(seal_size);p->after.reserve(seal_size);
 canonical->require_initial_sight_operands_without_callbacks(p->args);
 require(s.world.map_plots&&s.world.map_plots->plots.size()==(seal_size-22)/14,"Newborn sight map changed during storage allocation");
 sight_semantic(s,p->args.team,[&](auto value){p->before.push_back(static_cast<std::int64_t>(value));});
 require(p->before.size()==seal_size,"Newborn sight prepared semantic span changed");
 require(host.database().gameplay_fingerprint_sha256()==host.identity().rules_sha256,"Newborn sight activated rules changed");
 const auto rules=load_rules(host.database(),host.types(),host.defines());const auto unit=rules.units.find(p->args.type);
 require(unit!=rules.units.end(),"Newborn sight actual UnitInfo absent");
 const auto range=i32(unit->second.second+p->args.extra_visibility);require(range>=0&&range<=8,"Newborn sight native range unsupported");
 const auto extra=team_field(s.world.teams->teams.at(static_cast<std::size_t>(p->args.team)),"extra_water_see_from");require(extra>=0&&extra<=INT32_MAX,"Newborn sight water height invalid");
 MapGeometry geometry{s.world.map_header.grid,s.world.map_header.wrap_x,s.world.map_header.wrap_y,saves::GameStateLayout::expansion2_version1};geometry.validate();
 const auto center=geometry.plot(p->args.visual.x,p->args.visual.y);require(center.has_value(),"Newborn sight actual destination outside map");
 const auto calls=scan_adjacent_sight(s.world,geometry,static_cast<std::uint64_t>(*center),range,rules,extra>0);
 auto&cache=s.plot_visibility;p->cache_counts=cache.counts.data();p->cache_source=cache.source_raw.data();
 p->stores.reserve(calls.size());std::set<std::uint64_t> visited;
 for(const auto&call:calls){
  require(visited.insert(call.plot).second,"Newborn sight native ring repeated plot");
  auto&plot=s.world.map_plots->plots.at(static_cast<std::size_t>(call.plot));
  auto&visibility=sight_mutable_array(plot,"visibility");auto&invisible=sight_mutable_array(plot,"invisible_visibility");
  const auto team=static_cast<std::size_t>(p->args.team),slot=static_cast<std::size_t>(call.plot*80)+team;
  const auto before=cache.counts.at(slot),raw=visibility.values.at(team),inv=invisible.values.at(team);
  require(cache.source_raw.at(slot)==raw&&before==std::max<std::int64_t>(0,raw),"Newborn sight actual post-read visibility/cache diverged");
  require(before>0&&before<INT16_MAX,"Newborn sight zero edge requires actual reveal/exploration/meet/center receiver");
  require(inv>=0&&inv<=INT16_MAX,"Newborn sight invisible native short invalid");
  const bool based=call.ring<2; // CvPlot.h default based-on-unit=true EVEN with NO_INVISIBLE.
  if(based)require(inv>0&&inv<INT16_MAX,"Newborn sight invisible zero edge requires actual renderer/center receiver");
  p->stores.push_back({call.plot,slot,team,&visibility.values,&invisible.values,visibility.values.data(),invisible.values.data(),
   before,before+1,raw,inv,inv+(based?1:0),based});
 }
 require(!p->stores.empty(),"Newborn sight native center was not reached");
 require(s.world.map_plots&&s.world.map_plots->plots.size()==(seal_size-22)/14,"Newborn sight map changed during preparation");
 sight_semantic(s,p->args.team,[&](auto value){p->after.push_back(static_cast<std::int64_t>(value));},&p->stores);
 require(p->after.size()==seal_size,"Newborn sight prepared after span changed");
 auto result=std::shared_ptr<BnwInitialUnitSightOwner>(new BnwInitialUnitSightOwner(std::move(p)));
 canonical->require_initial_placement_current(result->impl_->args.visual.pending.get());
 result->require_before_without_callbacks(*canonical);return result;
}
void BnwInitialUnitSightOwner::require_before_without_callbacks(const BnwNativeUnitAllocatorCore&source)const{
 const auto&p=*impl_;require(!p.applied&&!p.core.owner_before(source.factory_self_)&&!source.factory_self_.owner_before(p.core),"Newborn sight actual Core lineage/phase differs");
 source.require_initial_sight_operands_without_callbacks(p.args);match_sight(*p.args.visual.state,p.args.team,p.before);
 const auto&s=*p.args.visual.state;require(s.plot_visibility.counts.data()==p.cache_counts&&s.plot_visibility.source_raw.data()==p.cache_source,
  "Newborn sight actual cache storage replaced");
 for(const auto&item:p.stores){const auto&plot=s.world.map_plots->plots.at(static_cast<std::size_t>(item.plot));
  const auto&v=sight_array(plot,"visibility");const auto&i=sight_array(plot,"invisible_visibility");
  require(&v.values==item.visibility&&&i.values==item.invisible&&v.values.data()==item.visibility_data&&i.values.data()==item.invisible_data,
   "Newborn sight actual array storage replaced");}
 source.require_initial_sight_operands_without_callbacks(p.args); // Final noalloc parent+node bookend.
}
void BnwInitialUnitSightOwner::apply_without_callbacks()noexcept{
 auto&p=*impl_;auto&cache=p.args.visual.state->plot_visibility;
 // ALL allocation and source checks completed by Core. Native ring order:
 // base count, NO_INVISIBLE individual branch absent, based-unit invisible.
 for(const auto&item:p.stores){(*item.visibility)[item.team]=item.after;cache.source_raw[item.cache_slot]=item.after;cache.counts[item.cache_slot]=item.after;
  if(item.based_on_unit)(*item.invisible)[item.team]=item.invisible_after;}
 p.applied=true;
}
void BnwInitialUnitSightOwner::require_after_without_callbacks(const BnwNativeUnitAllocatorCore&source)const{
 const auto&p=*impl_;require(p.applied&&!p.core.owner_before(source.factory_self_)&&!source.factory_self_.owner_before(p.core),"Newborn sight stored Core lineage missing");
 source.require_initial_sight_operands_without_callbacks(p.args);match_sight(*p.args.visual.state,p.args.team,p.after);
 source.require_initial_sight_operands_without_callbacks(p.args);
}
}

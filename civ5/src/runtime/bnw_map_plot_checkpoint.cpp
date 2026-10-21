#include "civ5/runtime/bnw_map_plot_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <limits>
#include <map>
#include <set>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','M','A','P','1'};
constexpr auto id=static_cast<std::uint32_t>(saves::CivisSaveSectionId::live_bnw_map_plots);
constexpr std::size_t v1_fixed_size=8+4+64+64+8*4;
constexpr std::size_t v1_record_size=8*5;
constexpr std::size_t v2_fixed_size=v1_fixed_size+8*4;
constexpr std::size_t v3_fixed_size=v2_fixed_size+8;
constexpr std::size_t v4_fixed_size=v3_fixed_size+8;
constexpr std::size_t v6_fixed_size=v4_fixed_size+16+8+64+8;
constexpr std::size_t v2_field_count=13;
constexpr std::size_t v4_field_count=17;
constexpr std::size_t field_count=21;
constexpr std::uint32_t v2_field_mask=(1u<<v2_field_count)-1;
constexpr std::uint32_t v4_field_mask=(1u<<v4_field_count)-1;
constexpr std::uint32_t field_mask=(1u<<field_count)-1;
void require(bool valid,const char *message) { if(!valid) throw std::runtime_error(message); }
// SDK CvPlot::read fields, Expansion2 CvPlot.cpp: plot versions 1..7.
constexpr std::array<std::string_view,field_count> names{
    "plot_type","terrain_type","owner","feature_type","improvement_type",
    "route_type","route_responsible","route_pillaged","improvement_pillaged",
    "rough_feature","feature_variety","improvement_builder","improvement_responsible",
    "ownership_duration","improvement_duration","world_anchor","world_anchor_data",
    "builder_scratch_player","builder_scratch_turn","builder_scratch_value",
    "builder_scratch_route"};
std::string_view name_for(const saves::PlotObservation &plot,std::size_t bit) {
    require(plot.version && *plot.version>=1 && *plot.version<=7,
            "Unsupported BNW plot wire version");
    if(bit==3 && *plot.version>=3) return "feature_type_hash";
    if(bit==4 && *plot.version>=5) return "improvement_type_hash";
    return names.at(bit);
}
bool supported_field(const saves::PlotObservation &plot,std::string_view name) {
    for(std::size_t bit=0;bit<field_count;++bit)
        if(name_for(plot,bit)==name) return true;
    return false;
}
void check_hash(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW map checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i16(std::int64_t value) {
        require(value>=INT16_MIN&&value<=INT16_MAX,"BNW yield cache exceeds signed-short wire range");
        const auto bits=static_cast<std::uint16_t>(value);
        bytes.push_back(static_cast<std::uint8_t>(bits));
        bytes.push_back(static_cast<std::uint8_t>(bits>>8));
    }
    void i64(std::int64_t value) { u64(static_cast<std::uint64_t>(value)); }
    void hash(std::string_view value) { bytes.insert(bytes.end(),value.begin(),value.end()); }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW map checkpoint");
        std::uint64_t result=0;
        for(unsigned i=0;i<8;++i) result|=std::uint64_t(bytes[at++])<<(8*i);
        return result;
    }
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW map checkpoint");
        std::uint32_t result=0;
        for(unsigned i=0;i<4;++i) result|=std::uint32_t(bytes[at++])<<(8*i);
        return result;
    }
    std::int16_t i16() {
        require(at<=bytes.size()&&bytes.size()-at>=2,"Truncated BNW yield cache record");
        const auto bits=std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8);at+=2;
        const auto value=bits<=INT16_MAX?static_cast<std::int32_t>(bits):static_cast<std::int32_t>(bits)-65536;
        return static_cast<std::int16_t>(value);
    }
    std::int64_t i64() { return static_cast<std::int64_t>(u64()); }
    void hash(std::string_view expected,const char *message) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),message);
        at+=expected.size();
    }
};
void map_shape(const saves::SavedSessionSnapshot &world) {
    const auto bnw=saves::GameStateLayout::expansion2_version1;
    require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.map_plots && world.map_projection && world.map_terrain &&
            world.map_plots->layout==bnw,"BNW map checkpoint requires complete map views");
    require(world.map_projection->grid.width==world.map_header.grid.width&&
            world.map_projection->grid.height==world.map_header.grid.height&&
            world.map_projection->wrap_x==world.map_header.wrap_x&&
            world.map_projection->wrap_y==world.map_header.wrap_y&&
            world.map_terrain->grid.width==world.map_header.grid.width&&
            world.map_terrain->grid.height==world.map_header.grid.height&&
            world.map_terrain->wrap_x==world.map_header.wrap_x&&
            world.map_terrain->wrap_y==world.map_header.wrap_y,
            "BNW map checkpoint derived grid/wrap identity changed");
    const auto size=world.map_header.grid.size();
    require(size<=static_cast<std::uint64_t>(INT64_MAX),
            "BNW map checkpoint exceeds signed counter range");
    require(world.map_plots->plots.size()==size && world.map_projection->cells.size()==size &&
            world.map_terrain->cells.size()==size,"BNW map checkpoint cell count mismatch");
    for(std::uint64_t i=0;i<size;++i) {
        const auto &raw=world.map_plots->plots[static_cast<std::size_t>(i)];
        const auto &cell=world.map_projection->cells[static_cast<std::size_t>(i)];
        const auto xy=world.map_header.grid.coordinates(i);
        require(raw.index==i && raw.x==xy[0] && raw.y==xy[1] &&
                cell.index==i && cell.x==xy[0] && cell.y==xy[1],
                "BNW map checkpoint plot ID or coordinate mismatch");
    }
}
std::int64_t field(const saves::PlotObservation &plot,std::string_view name) {
    const std::int64_t *found=nullptr;
    for(const auto &candidate:plot.fields)
        if(candidate.name && name==candidate.name) {
            require(!found,"Duplicate BNW map plot field");
            found=&candidate.value;
        }
    require(found,"Missing BNW map plot field");
    return *found;
}
void validate_field_value(std::string_view name,std::int64_t value) {
    if(name=="feature_type_hash" || name=="improvement_type_hash")
        require(value>=0 && value<=UINT32_MAX,"BNW map plot hash exceeds uint32 wire range");
    else if(name=="ownership_duration" || name=="improvement_duration")
        require(value>=0 && value<=INT16_MAX,"BNW map plot duration exceeds signed-short wire range");
    else if(name=="builder_scratch_turn" || name=="builder_scratch_value")
        require(value>=INT16_MIN && value<=INT16_MAX,
                "BNW builder scratch field exceeds signed-short wire range");
    else if(name=="builder_scratch_route")
        require(value>=-1 && value<=INT32_MAX,"BNW builder scratch route exceeds i32 wire range");
    else if(name=="route_pillaged" || name=="improvement_pillaged" || name=="rough_feature")
        require(value==0 || value==1,"BNW map plot flag is not boolean");
    else
        require(value>=-128 && value<=127,"BNW map plot scalar exceeds signed-byte wire range");
    if(name=="plot_type")
        require(value>=0 && value<=3,"BNW map plot type is invalid");
}
void set_field(saves::PlotObservation &plot,std::string_view name,std::int64_t value) {
    validate_field_value(name,value);
    saves::NamedSaveInteger *found=nullptr;
    for(auto &candidate:plot.fields)
        if(candidate.name && name==candidate.name) {
            require(!found,"Duplicate BNW map plot field");
            found=&candidate;
        }
    require(found,"Missing BNW map plot field");
    found->value=value;
}
bool raw_other_fields_same(const saves::PlotObservation &a,const saves::PlotObservation &b) {
    if(a.index!=b.index||a.x!=b.x||a.y!=b.y||a.version!=b.version||
       a.start_offset!=b.start_offset||a.end_offset!=b.end_offset||
       a.fields.size()!=b.fields.size()||a.arrays.size()!=b.arrays.size()||
       a.script_data!=b.script_data||a.legacy_build_progress.size()!=b.legacy_build_progress.size()||
       a.invisible_visibility.size()!=b.invisible_visibility.size()) return false;
    for(std::size_t i=0;i<a.fields.size();++i) {
        const auto name=std::string_view(a.fields[i].name?a.fields[i].name:"");
        if(name!=std::string_view(b.fields[i].name?b.fields[i].name:"") ||
           (!supported_field(a,name) && a.fields[i].value!=b.fields[i].value))
            return false;
    }
    for(std::size_t i=0;i<a.arrays.size();++i) {
        const auto &x=a.arrays[i],&y=b.arrays[i];
        const auto name=std::string_view(x.name?x.name:"");
        if(name!=std::string_view(y.name?y.name:"") ||
           x.columns!=y.columns || x.values.size()!=y.values.size() ||
           (name!="visibility" && name!="yields" && x.values!=y.values)) return false;
    }
    for(std::size_t i=0;i<a.legacy_build_progress.size();++i)
        if(a.legacy_build_progress[i].type_hash!=b.legacy_build_progress[i].type_hash||
           a.legacy_build_progress[i].count!=b.legacy_build_progress[i].count) return false;
    for(std::size_t i=0;i<a.invisible_visibility.size();++i) {
        const auto &x=a.invisible_visibility[i],&y=b.invisible_visibility[i];
        if(x.key!=y.key||x.wire_count!=y.wire_count||x.values!=y.values) return false;
    }
    return true;
}
const saves::PlotArrayObservation &visibility(const saves::PlotObservation &plot,
                                              std::uint64_t teams) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:plot.arrays)
        if(array.name && std::string_view(array.name)=="visibility") {
            require(!found,"Duplicate BNW plot visibility array");
            found=&array;
        }
    require(found && found->columns==1 && found->values.size()==teams && teams>0,
            "BNW plot visibility team dimensions changed");
    return *found;
}
saves::PlotArrayObservation &mutable_visibility(saves::PlotObservation &plot,
                                                std::uint64_t teams) {
    (void)visibility(plot,teams);
    for(auto &array:plot.arrays)
        if(array.name && std::string_view(array.name)=="visibility") return array;
    throw std::runtime_error("Missing BNW plot visibility array");
}
std::array<std::int16_t,6> yield_cache(const saves::SavedSessionSnapshot& world,std::uint64_t index) {
    require(world.map_plots->dimensions.yields==6,"BNW map checkpoint native yield dimension changed");
    const auto& plot=world.map_plots->plots.at(static_cast<std::size_t>(index));
    const saves::PlotArrayObservation* found=nullptr;
    for(const auto& array:plot.arrays)if(array.name&&std::string_view(array.name)=="yields") {
        require(!found,"Duplicate BNW plot yield array");found=&array;
    }
    require(found&&found->columns==1&&found->values.size()==6,
            "BNW map checkpoint requires exactly six short yields");
    const auto& live=world.map_projection->cells.at(static_cast<std::size_t>(index)).cached_yields;
    require(live&&live->size()==6,"BNW map checkpoint projected yield cache shape differs");
    std::array<std::int16_t,6> out{};
    for(std::size_t y=0;y<6;++y) {
        const auto value=found->values[y];const auto mirror=live->find(static_cast<std::int64_t>(y));
        require(value>=INT16_MIN&&value<=INT16_MAX,"BNW yield cache exceeds signed-short wire range");
        require(mirror!=live->end()&&mirror->second==value,"BNW yield cache raw/projected mirror differs");
        out[y]=static_cast<std::int16_t>(value);
    }
    return out;
}
std::string yield_baseline(const saves::SavedSessionSnapshot& world) {
    Writer encoded;
    for(std::uint64_t i=0;i<world.map_header.grid.size();++i)
        for(const auto value:yield_cache(world,i))encoded.i16(value);
    return identify_bytes(encoded.bytes).sha256;
}
void set_yield_cache(saves::PlotObservation& plot,const std::array<std::int16_t,6>& values) {
    saves::PlotArrayObservation* found=nullptr;
    for(auto& array:plot.arrays)if(array.name&&std::string_view(array.name)=="yields") {
        require(!found,"Duplicate BNW plot yield array");found=&array;
    }
    require(found&&found->columns==1&&found->values.size()==6,"BNW restored yield cache shape differs");
    for(std::size_t y=0;y<6;++y)found->values[y]=values[y];
}
void validate_visibility_change(std::int64_t base,std::int64_t live) {
    // Expansion2 CvPlot::changeVisibilityCount changes revelation, territory,
    // meetings and UI when isVisible changes. This codec only covers a count
    // change that stays visible; the remaining effects require another section.
    require(base>0 && base<=INT16_MAX && live>0 && live<=INT16_MAX && base!=live,
            "BNW visibility transition needs uncheckpointed reveal effects");
}
std::optional<std::vector<bool>> assert_projection(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedTerrainFeatureResolver &features) {
    const auto projected=saves::project_saved_map(world.map_header,*world.map_plots,features);
    const auto terrain=saves::project_saved_map_terrain(projected);
    // CvCity::Read resolves plot-city handles after the raw map projection is
    // built. A live session therefore has a derived is_city bit that the raw
    // plot bytes alone cannot reproduce. If that hook has run, validate all
    // city bits against the saved city roster rather than accepting a partial
    // or stale projection.
    bool live_cities=false;
    for(std::size_t i=0;i<projected.cells.size();++i)
        live_cities|=projected.cells[i].is_city!=
                     world.map_projection->cells[i].is_city;
    std::vector<bool> city_mask;
    if(live_cities) {
        require(world.all_players.has_value(),
                "BNW live city projection requires saved players");
        city_mask.assign(projected.cells.size(),false);
        const auto city_scalar=[](const saves::CityCoreObservation &core,
                                  std::string_view wanted) {
            const saves::CityCoreField *found=nullptr;
            for(const auto &entry:core.fields)
                if(entry.name && wanted==entry.name) {
                    require(!found,"Duplicate BNW live city identity field");
                    found=&entry;
                }
            require(found && found->values.size()==1,
                    "BNW live city identity field is missing");
            return found->values.front();
        };
        for(const auto &player:world.all_players->players)
            for(const auto &city:player.cities.cities) {
                const auto owner=city_scalar(city.core,"m_eOwner");
                const auto id=city_scalar(city.core,"m_iID");
                const auto x=city_scalar(city.core,"m_iX");
                const auto y=city_scalar(city.core,"m_iY");
                require(owner==player.saved_player_id && id>=0 && x>=0 && y>=0 &&
                        static_cast<std::uint64_t>(x)<world.map_header.grid.width &&
                        static_cast<std::uint64_t>(y)<world.map_header.grid.height,
                        "BNW live city identity is outside saved map");
                const auto index=world.map_header.grid.index(
                    static_cast<std::uint64_t>(x),static_cast<std::uint64_t>(y));
                const auto &cell=world.map_projection->cells.at(
                    static_cast<std::size_t>(index));
                require(cell.plot_city && cell.plot_city->owner==owner &&
                        cell.plot_city->id==id &&
                        !city_mask[static_cast<std::size_t>(index)],
                        "BNW live city plot link disagrees with saved city");
                city_mask[static_cast<std::size_t>(index)]=true;
            }
    }
    for(std::size_t i=0;i<projected.cells.size();++i) {
        const auto &a=projected.cells[i],&b=world.map_projection->cells[i];
        require(a.index==b.index && a.x==b.x && a.y==b.y &&
                a.owner==b.owner && a.plot_type==b.plot_type &&
                a.terrain_type==b.terrain_type && a.feature_type==b.feature_type &&
                a.resource_type==b.resource_type && a.improvement_type==b.improvement_type &&
                a.continent_type==b.continent_type &&
                a.feature_type_hash==b.feature_type_hash &&
                a.resource_type_hash==b.resource_type_hash &&
                a.improvement_type_hash==b.improvement_type_hash &&
                (live_cities?b.is_city==city_mask[i]:a.is_city==b.is_city) &&
                a.rivers==b.rivers && a.route==b.route &&
                a.landmass==b.landmass &&
                a.improvement_pillaged==b.improvement_pillaged &&
                a.improvement_passable==b.improvement_passable &&
                a.water_cache==b.water_cache &&
                a.force_resource_reveal==b.force_resource_reveal &&
                a.artifact_era==b.artifact_era &&
                a.trade_route_units==b.trade_route_units &&
                a.river_crossing_bits==b.river_crossing_bits &&
                a.yield_city==b.yield_city && a.plot_city==b.plot_city &&
                a.yield_city_override==b.yield_city_override &&
                a.units==b.units && a.revealed_teams==b.revealed_teams &&
                a.potential_city_work==b.potential_city_work &&
                a.cached_yields==b.cached_yields &&
                a.profile_extensions.values.empty() && b.profile_extensions.values.empty() &&
                a.native_terrain_mask==b.native_terrain_mask &&
                a.native_terrain_mask_known==b.native_terrain_mask_known &&
                a.native_category==b.native_category && a.auxiliary==b.auxiliary,
                "BNW map checkpoint raw and projected topography disagree");
    }
    require(terrain.known_terrain_masks==world.map_terrain->known_terrain_masks &&
            terrain.cells.size()==world.map_terrain->cells.size(),
            "BNW map checkpoint raw and terrain views disagree");
    for(std::size_t i=0;i<terrain.cells.size();++i)
        require(terrain.cells[i].native_terrain_mask==world.map_terrain->cells[i].native_terrain_mask &&
                terrain.cells[i].native_terrain_mask_known==world.map_terrain->cells[i].native_terrain_mask_known &&
                terrain.cells[i].native_category==world.map_terrain->cells[i].native_category &&
                terrain.cells[i].auxiliary==world.map_terrain->cells[i].auxiliary,
                "BNW map checkpoint raw and terrain views disagree");
    if(live_cities)return city_mask;
    return std::nullopt;
}
bool known_slot(const content::RulesDatabase &rules,std::string_view table,std::int64_t slot) {
    if(slot<0) return false;
    bool found=false;
    const auto result=rules.prefetch_info(std::string(table),[&](std::uint64_t index,std::int64_t,const auto &){
        if(index==static_cast<std::uint64_t>(slot)) found=true;
        return true;
    });
    require(result.all_succeeded(),"BNW map checkpoint cannot read active type rules");
    return found;
}
std::int64_t checked_owned(std::int64_t base,std::int64_t delta,std::uint64_t size) {
    require(base>=0 && base<=static_cast<std::int64_t>(size) &&
            (delta>=0 ? base<=INT64_MAX-delta : base>=-delta),
            "BNW map checkpoint owned-plot counter overflow");
    const auto result=base+delta;
    require(result>=0 && result<=static_cast<std::int64_t>(size),
            "BNW map checkpoint owned-plot count is outside map");
    return result;
}
std::int64_t land_score(const saves::SavedPlayerObservation &player) {
    const saves::PlayerCoreField *found=nullptr;
    for(const auto &field:player.core.fields)
        if(field.name && std::string_view(field.name)=="m_iTotalLandScored") {
            require(!found,"Duplicate BNW player land score");
            found=&field;
        }
    require(found && found->encoding==saves::PlayerCoreEncoding::i32 &&
            found->values.size()==1 && found->values[0]>=0 &&
            found->values[0]<=INT32_MAX,"Invalid BNW player land score");
    return found->values[0];
}
void set_land_score(saves::SavedPlayerObservation &player,std::int64_t value) {
    require(value>=0 && value<=INT32_MAX,"BNW player land score exceeds i32 wire range");
    saves::PlayerCoreField *found=nullptr;
    for(auto &field:player.core.fields)
        if(field.name && std::string_view(field.name)=="m_iTotalLandScored") {
            require(!found,"Duplicate BNW player land score");
            found=&field;
        }
    require(found && found->encoding==saves::PlayerCoreEncoding::i32 &&
            found->values.size()==1,"Missing BNW player land score");
    found->values[0]=value;
}
struct ScoreChange {std::int64_t id=0,base=0,live=0;};
std::vector<ScoreChange> player_score_changes(const saves::SavedSessionSnapshot &source,
                                               const saves::SavedSessionSnapshot &world) {
    require(source.all_players && world.all_players &&
            source.all_players->layout==saves::GameStateLayout::expansion2_version1 &&
            world.all_players->layout==source.all_players->layout &&
            source.all_players->start_offset==world.all_players->start_offset &&
            source.all_players->end_offset==world.all_players->end_offset &&
            source.all_players->players.size()==world.all_players->players.size(),
            "BNW map checkpoint player roster changed");
    std::set<std::int64_t> seen;
    std::vector<ScoreChange> changed;
    for(std::size_t i=0;i<source.all_players->players.size();++i) {
        const auto &a=source.all_players->players[i],&b=world.all_players->players[i];
        require(a.index==b.index && a.start_offset==b.start_offset &&
                a.end_offset==b.end_offset && a.saved_player_id==b.saved_player_id &&
                a.saved_player_id>=0 && seen.insert(a.saved_player_id).second &&
                a.core.layout==b.core.layout &&
                a.core.start_offset==b.core.start_offset &&
                a.core.end_offset==b.core.end_offset &&
                a.core.yield_slots==b.core.yield_slots &&
                a.core.version==b.core.version &&
                a.core.fields.size()==b.core.fields.size(),
                "BNW map checkpoint player identity or core shape changed");
        for(std::size_t j=0;j<a.core.fields.size();++j) {
            const auto &x=a.core.fields[j],&y=b.core.fields[j];
            const auto name=std::string_view(x.name?x.name:"");
            require(name==std::string_view(y.name?y.name:"") &&
                    x.encoding==y.encoding && x.start_offset==y.start_offset &&
                    x.end_offset==y.end_offset && x.wire_count==y.wire_count &&
                    x.row_counts==y.row_counts && x.hashes==y.hashes && x.bytes==y.bytes &&
                    (name=="m_iTotalLandScored" || x.values==y.values),
                    "BNW map checkpoint has unsupported player-core mutation");
        }
        const auto base=land_score(a),live=land_score(b);
        if(base!=live) changed.push_back({a.saved_player_id,base,live});
    }
    std::sort(changed.begin(),changed.end(),[](const auto &a,const auto &b){return a.id<b.id;});
    return changed;
}
std::int64_t score_threshold(const content::GlobalDefineCache *defines) {
    require(defines && defines->profile()==content::GlobalDefineProfile::bnw,
            "BNW map checkpoint ownership threshold requires active defines");
    const auto *entry=defines->find("OWNERSHIP_SCORE_DURATION_THRESHOLD");
    require(entry && entry->lookup.found && entry->lookup.error.empty() &&
            std::holds_alternative<std::int64_t>(entry->lookup.value),
            "BNW map checkpoint ownership threshold is unavailable");
    const auto value=std::get<std::int64_t>(entry->lookup.value);
    require(value>0 && value<=INT16_MAX,
            "BNW map checkpoint ownership threshold is invalid");
    return value;
}
void validate_land_scores(const saves::SavedSessionSnapshot &source,
                          const saves::SavedSessionSnapshot &world,
                          const std::vector<ScoreChange> &scores,
                          const content::GlobalDefineCache *defines) {
    std::map<std::int64_t,std::int64_t> expected;
    bool duration_changed=false;
    for(std::size_t i=0;i<source.map_plots->plots.size();++i) {
        const auto &a=source.map_plots->plots[i],&b=world.map_plots->plots[i];
        const auto old_duration=field(a,"ownership_duration");
        const auto new_duration=field(b,"ownership_duration");
        if(old_duration==new_duration) continue;
        duration_changed=true;
        require(old_duration>=0 && old_duration<=INT16_MAX &&
                new_duration>=0 && new_duration<=INT16_MAX,
                "BNW ownership duration exceeds i16 wire range");
    }
    if(!duration_changed && scores.empty()) return;
    const auto threshold=score_threshold(defines);
    for(std::size_t i=0;i<source.map_plots->plots.size();++i) {
        const auto &a=source.map_plots->plots[i],&b=world.map_plots->plots[i];
        const auto old_duration=field(a,"ownership_duration");
        const auto new_duration=field(b,"ownership_duration");
        if(old_duration==new_duration) continue;
        const auto old_owner=field(a,"owner"),new_owner=field(b,"owner");
        require(old_owner==new_owner && (new_duration>=old_duration || new_owner==-1),
                "BNW map checkpoint ownership reset needs full owner setter effects");
        if(new_owner<0 || field(b,"plot_type")==3) continue;
        if(old_duration<threshold && new_duration>=threshold) ++expected[new_owner];
        if(old_duration>=threshold && new_duration<threshold) --expected[new_owner];
    }
    std::map<std::int64_t,std::int64_t> actual;
    for(const auto &score:scores) actual[score.id]=score.live-score.base;
    require(actual==expected,"BNW player land score does not match plot duration crossings");
}
bool rough_feature(const content::RulesDatabase &rules,std::int64_t slot) {
    bool found=false,rough=false;
    const auto result=rules.prefetch_info("Features",[&](std::uint64_t index,std::int64_t,
                                                          const content::RulesRowView &row){
        if(index==static_cast<std::uint64_t>(slot)) {
            found=true;rough=row.boolean("Rough");
        }
        return true;
    });
    require(result.all_succeeded() && found,"BNW map checkpoint feature Rough rule unresolved");
    return rough;
}
void validate_changed_types(const saves::PlotObservation &base,
                            const saves::PlotObservation &live,
                            const saves::SavedMapCellProjection &base_cell,
                            const saves::SavedMapCellProjection &live_cell,
                            const saves::SavedTerrainFeatureResolver &features,
                            const content::RulesDatabase *rules,
                            const content::InfoTypeCatalog *types,
                            std::uint64_t player_slots,
                            const saves::AllPlayersObservation *players) {
    const auto bp=field(base,"plot_type"),lp=field(live,"plot_type");
    require(bp>=0 && bp<=3 && lp>=0 && lp<=3 && (bp==3)==(lp==3),
            "BNW map checkpoint land/water transition needs full CvPlot setter effects");
    const auto owner=field(live,"owner");
    require(owner>=-1 && (owner==-1 || static_cast<std::uint64_t>(owner)<player_slots),
            "BNW map checkpoint owner is outside player slots");
    if(field(base,"owner")!=owner) {
        require(!base_cell.is_city && (!base_cell.units || base_cell.units->empty()),
                "BNW map checkpoint city/unit ownership needs full setter effects");
        if(owner>=0) {
            require(players && std::any_of(players->players.begin(),players->players.end(),
                    [&](const auto &player){return player.saved_player_id==owner;}),
                    "BNW map checkpoint owner is absent from restored players");
        }
    }
    const auto feature_name=name_for(live,3),improvement_name=name_for(live,4);
    const auto feature=field(live,feature_name),improvement=field(live,improvement_name);
    const bool feature_changed=field(base,feature_name)!=feature;
    const bool improvement_changed=field(base,improvement_name)!=improvement;
    const auto route=field(live,"route_type");
    const bool route_changed=field(base,"route_type")!=route;
    if(feature_changed || improvement_changed || route_changed) {
        require(rules && types && types->report().all_succeeded(),
                "BNW map checkpoint type mutation requires active rules/catalog");
    }
    if(feature_changed) {
        std::int64_t feature_slot=-1;
        if(feature_name=="feature_type_hash" && feature!=0) {
            const auto *entry=types->find_hash(static_cast<std::uint64_t>(feature));
            require(entry && entry->table=="Features" &&
                    known_slot(*rules,"Features",entry->id),"Unknown BNW feature hash");
            feature_slot=entry->id;
        }
        if(feature_name=="feature_type" && feature!=-1) {
            require(known_slot(*rules,"Features",feature),"Unknown BNW feature ID");
            feature_slot=feature;
        }
        const auto old_none=(base_cell.feature_type_hash==0 || base_cell.feature_type==-1);
        const auto new_none=(live_cell.feature_type_hash==0 || live_cell.feature_type==-1);
        const auto old_info=old_none?std::optional<saves::SavedTerrainFeature>{}:
            features?features(base_cell):std::nullopt;
        const auto new_info=new_none?std::optional<saves::SavedTerrainFeature>{}:
            features?features(live_cell):std::nullopt;
        require((old_none||old_info) && (new_none||new_info) &&
                (old_info?old_info->natural_wonder:false)==
                    (new_info?new_info->natural_wonder:false),
                "BNW map checkpoint natural-wonder transition or feature resolution unavailable");
        require(field(live,"feature_variety")==0,
                "BNW feature setter requires zero variety");
        require(field(live,"rough_feature")==
                    (feature_slot==-1?0:rough_feature(*rules,feature_slot)),
                "BNW feature rough flag disagrees with active rules");
    }
    if(improvement_changed) {
        if(improvement_name=="improvement_type_hash" && improvement!=0) {
            const auto *entry=types->find_hash(static_cast<std::uint64_t>(improvement));
            require(entry && entry->table=="Improvements" &&
                    known_slot(*rules,"Improvements",entry->id),
                    "Unknown BNW improvement hash");
        }
        if(improvement_name=="improvement_type" && improvement!=-1)
            require(known_slot(*rules,"Improvements",improvement),"Unknown BNW improvement ID");
    }
    if(route_changed && route!=-1)
        require(known_slot(*rules,"Routes",route),"Unknown BNW route ID");
    if(route_changed)
        require(field(live,"route_pillaged")==0,
                "BNW route setter must clear pillaged state");
    require(route==-1 || route>=0,
            "BNW map checkpoint route is invalid");
}
void validate_builder_scratch(const saves::PlotObservation &base,
                              const saves::PlotObservation &live,
                              const content::RulesDatabase *rules,
                              const content::InfoTypeCatalog *types,
                              const saves::AllPlayersObservation *players) {
    bool changed=false;
    for(std::size_t bit=v4_field_count;bit<field_count;++bit)
        changed|=field(base,names[bit])!=field(live,names[bit]);
    if(!changed) return;
    // Expansion2 CvPlot.cpp SetBuilderAIScratchPad* only assign these four
    // fields. CvBuilderTaskingAI.cpp writes them together for a candidate.
    require(rules && types && types->report().all_succeeded() && players,
            "BNW builder scratch checkpoint requires active rules and roster");
    const auto player=field(live,"builder_scratch_player");
    const auto route=field(live,"builder_scratch_route");
    require(player>=-1 &&
            (player==-1 || std::any_of(players->players.begin(),players->players.end(),
                [&](const auto &item){return item.saved_player_id==player;})),
            "BNW builder scratch player is absent from roster");
    require(route==-1 || (player>=0 && known_slot(*rules,"Routes",route)),
            "BNW builder scratch route is absent from active rules");
    for(std::size_t bit=v4_field_count;bit<field_count;++bit)
        validate_field_value(names[bit],field(live,names[bit]));
}
} // namespace

saves::CivisSaveSection make_bnw_map_plot_checkpoint(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules,const content::InfoTypeCatalog *types,
    const content::GlobalDefineCache *defines) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    map_shape(source);map_shape(world);
    const auto size=source.map_header.grid.size();
    const auto& base_dimensions=source.map_plots->dimensions;
    const auto& live_dimensions=world.map_plots->dimensions;
    require(base_dimensions.players==live_dimensions.players&&
            base_dimensions.teams==live_dimensions.teams&&
            base_dimensions.major_civilizations==live_dimensions.major_civilizations&&
            base_dimensions.yields==live_dimensions.yields&&
            base_dimensions.invisible_types==live_dimensions.invisible_types&&
            source.map_plots->start_offset==world.map_plots->start_offset&&
            source.map_plots->end_offset==world.map_plots->end_offset,
            "BNW map checkpoint raw map metadata changed");
    require(world.map_header.grid.width==source.map_header.grid.width &&
            world.map_header.grid.height==source.map_header.grid.height &&
            world.map_header.wrap_x==source.map_header.wrap_x &&
            world.map_header.wrap_y==source.map_header.wrap_y &&
            world.map_header.land_plots==source.map_header.land_plots &&
            world.map_header.natural_wonders==source.map_header.natural_wonders&&
            world.map_header.guid==source.map_header.guid,
            "BNW map checkpoint source header changed");
    assert_projection(source,features);
    assert_projection(world,features);
    const auto scores=player_score_changes(source,world);
    validate_land_scores(source,world,scores,defines);
    Writer out;out.bytes.reserve(v6_fixed_size);
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(6);
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.u64(source.map_header.grid.width);out.u64(source.map_header.grid.height);out.u64(size);
    const auto count_offset=out.bytes.size();out.u64(0);
    out.i64(source.map_header.land_plots);out.i64(world.map_header.land_plots);
    out.i64(source.map_header.owned_plots);out.i64(world.map_header.owned_plots);
    out.u64(scores.size());
    const auto visibility_count_offset=out.bytes.size();out.u64(0);
    out.bytes.insert(out.bytes.end(),source.map_header.guid.begin(),source.map_header.guid.end());
    out.u64(static_cast<std::uint64_t>(source.map_header.wrap_x)|(static_cast<std::uint64_t>(source.map_header.wrap_y)<<1));
    out.hash(yield_baseline(source));
    const auto yield_count_offset=out.bytes.size();out.u64(0);
    std::uint64_t changed=0;
    std::int64_t owned_delta=0;
    for(std::uint64_t i=0;i<size;++i) {
        const auto &base=source.map_plots->plots[static_cast<std::size_t>(i)];
        const auto &live=world.map_plots->plots[static_cast<std::size_t>(i)];
        (void)yield_cache(source,i);(void)yield_cache(world,i);
        require(raw_other_fields_same(base,live),"BNW map checkpoint has unsupported plot mutation");
        std::uint32_t mask=0;
        for(std::size_t bit=0;bit<field_count;++bit)
            if(field(base,name_for(base,bit))!=field(live,name_for(live,bit))) mask|=1u<<bit;
        if(!mask) continue;
        validate_changed_types(base,live,source.map_projection->cells[static_cast<std::size_t>(i)],
                               world.map_projection->cells[static_cast<std::size_t>(i)],
                               features,rules,types,world.map_plots->dimensions.players,
                               world.all_players?&*world.all_players:nullptr);
        validate_builder_scratch(base,live,rules,types,
                                 world.all_players?&*world.all_players:nullptr);
        const auto old_owner=field(base,"owner"),new_owner=field(live,"owner");
        owned_delta+=(new_owner>=0)-(old_owner>=0);
        out.u64(i);out.u32(mask);
        for(std::size_t bit=0;bit<field_count;++bit) if(mask&(1u<<bit)) {
            const auto name=name_for(live,bit);
            const auto value=field(live,name);
            validate_field_value(name,value);
            out.i64(field(base,name_for(base,bit)));
            out.i64(value);
        }
        ++changed;
    }
    require(world.map_header.owned_plots==
            checked_owned(source.map_header.owned_plots,owned_delta,size),
            "BNW map checkpoint owned-plot counter disagrees with owner changes");
    for(unsigned i=0;i<8;++i)
        out.bytes[count_offset+i]=static_cast<std::uint8_t>(changed>>(8*i));
    for(const auto &score:scores) {
        out.i64(score.id);out.i64(score.base);out.i64(score.live);
    }
    std::uint64_t visibility_changes=0;
    const auto teams=source.map_plots->dimensions.teams;
    require(teams==world.map_plots->dimensions.teams,
            "BNW map checkpoint team dimensions changed");
    for(std::uint64_t i=0;i<size;++i) {
        const auto &base=visibility(source.map_plots->plots[static_cast<std::size_t>(i)],teams);
        const auto &live=visibility(world.map_plots->plots[static_cast<std::size_t>(i)],teams);
        for(std::uint64_t team=0;team<teams;++team) {
            const auto old=base.values[static_cast<std::size_t>(team)];
            const auto now=live.values[static_cast<std::size_t>(team)];
            if(old==now) continue;
            validate_visibility_change(old,now);
            out.u64(i);out.u64(team);out.i64(old);out.i64(now);
            ++visibility_changes;
        }
    }
    for(unsigned i=0;i<8;++i)
        out.bytes[visibility_count_offset+i]=static_cast<std::uint8_t>(visibility_changes>>(8*i));
    std::uint64_t yield_changes=0;
    for(std::uint64_t i=0;i<size;++i) {
        const auto base=yield_cache(source,i),live=yield_cache(world,i);
        if(base==live)continue;
        out.u64(i);for(const auto value:base)out.i16(value);for(const auto value:live)out.i16(value);
        ++yield_changes;
    }
    for(unsigned i=0;i<8;++i)
        out.bytes[yield_count_offset+i]=static_cast<std::uint8_t>(yield_changes>>(8*i));
    return {id,0,std::move(out.bytes)};
}

void restore_bnw_map_plot_checkpoint(
    const saves::CivisSaveSection &section,saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity,const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules,const content::InfoTypeCatalog *types,
    const content::GlobalDefineCache *defines) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    require(section.id==id&&section.flags==0&&section.bytes.size()>=v1_fixed_size&&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW map checkpoint section");
    map_shape(world);
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require(version>=1 && version<=6,"Unsupported BNW map checkpoint version");
    in.hash(identity.payload_sha256,"BNW map checkpoint source payload changed");
    in.hash(identity.rules_sha256,"BNW map checkpoint active rules changed");
    const auto width=in.u64(),height=in.u64(),size=in.u64(),count=in.u64();
    require(width==world.map_header.grid.width&&height==world.map_header.grid.height&&
            size==world.map_header.grid.size()&&count<=size,
            "BNW map checkpoint dimensions or record count changed");
    std::int64_t base_land=world.map_header.land_plots,live_land=base_land;
    std::int64_t base_owned=world.map_header.owned_plots,live_owned=base_owned;
    std::uint64_t player_count=0;
    std::uint64_t visibility_count=0,yield_count=0;
    std::optional<std::vector<bool>> live_city_mask;
    if(version==1)
        require(count==(section.bytes.size()-v1_fixed_size)/v1_record_size&&
                (section.bytes.size()-v1_fixed_size)%v1_record_size==0,
                "BNW v1 map checkpoint record count changed");
    else {
        require(section.bytes.size()>=v2_fixed_size,
                "Truncated BNW v2 map checkpoint header");
        base_land=in.i64();live_land=in.i64();
        base_owned=in.i64();live_owned=in.i64();
        require(base_land==world.map_header.land_plots &&
                base_owned==world.map_header.owned_plots &&
                live_land==base_land && base_owned>=0 && live_owned>=0 &&
                base_owned<=static_cast<std::int64_t>(size) &&
                live_owned<=static_cast<std::int64_t>(size),
                "BNW map checkpoint source header or counters changed");
        if(version>=3) {
            require(section.bytes.size()>=v3_fixed_size && world.all_players,
                    "Truncated BNW v3 map checkpoint players");
            player_count=in.u64();
            require(player_count<=world.all_players->players.size(),
                    "BNW map checkpoint player score count is invalid");
        }
        if(version>=4) {
            require(section.bytes.size()>=v4_fixed_size,
                    "Truncated BNW v4 map checkpoint visibility");
            visibility_count=in.u64();
            const auto teams=world.map_plots->dimensions.teams;
            require(teams>0 && size<=UINT64_MAX/teams &&
                    visibility_count<=size*teams,
                    "BNW map checkpoint visibility record count is invalid");
        }
    }
    if(version>=6) {
        require(section.bytes.size()>=v6_fixed_size,"Truncated BNW v6 yield cache header");
        require(in.at<=section.bytes.size()&&section.bytes.size()-in.at>=16&&
                std::equal(world.map_header.guid.begin(),world.map_header.guid.end(),section.bytes.begin()+in.at),
                "BNW map checkpoint source GUID changed");in.at+=16;
        const auto wrap=in.u64();
        require(wrap==(static_cast<std::uint64_t>(world.map_header.wrap_x)|(static_cast<std::uint64_t>(world.map_header.wrap_y)<<1)),
                "BNW map checkpoint source wrap changed");
        live_city_mask=assert_projection(world,features);
        in.hash(yield_baseline(world),"BNW map checkpoint source yield-cache baseline changed");
        yield_count=in.u64();require(yield_count<=size,"BNW map checkpoint yield record count changed");
    }
    auto replacement=world;
    std::uint64_t previous=0;
    std::int64_t owned_delta=0;
    std::vector<std::uint64_t> changed_indices;
    for(std::uint64_t n=0;n<count;++n) {
        const auto index=in.u64();
        require(index<size&&(n==0||index>previous),"BNW map checkpoint plot IDs are not unique and ordered");
        previous=index;
        auto &plot=replacement.map_plots->plots[static_cast<std::size_t>(index)];
        if(version==1) {
            const auto bp=in.i64(),bt=in.i64(),lp=in.i64(),lt=in.i64();
            require(field(plot,"plot_type")==bp&&field(plot,"terrain_type")==bt,
                    "BNW map checkpoint source plot changed");
            require(bp!=lp||bt!=lt,"BNW map checkpoint contains an unchanged plot");
            require(bp>=0 && bp<=3 && lp>=0 && lp<=3 && (bp==3)==(lp==3),
                    "BNW v1 map checkpoint needs unsupported land/water effects");
            set_field(plot,"plot_type",lp);set_field(plot,"terrain_type",lt);
        } else {
            const auto mask=in.u32();
            require(mask && !(mask&~(version==2?v2_field_mask:
                                      version<=4?v4_field_mask:field_mask)),
                    "BNW map checkpoint field mask is invalid");
            const auto old_owner=field(plot,"owner");
            for(std::size_t bit=0;bit<field_count;++bit) if(mask&(1u<<bit)) {
                const auto base=in.i64(),live=in.i64();
                const auto name=name_for(plot,bit);
                require(base!=live && field(plot,name)==base,
                        "BNW map checkpoint source field changed");
                set_field(plot,name,live);
            }
            owned_delta+=(field(plot,"owner")>=0)-(old_owner>=0);
        }
        changed_indices.push_back(index);
    }
    std::int64_t previous_player=-1;
    for(std::uint64_t n=0;n<player_count;++n) {
        const auto player_id=in.i64(),base=in.i64(),live=in.i64();
        require(player_id>previous_player && base!=live &&
                base>=0 && base<=INT32_MAX && live>=0 && live<=INT32_MAX,
                "BNW map checkpoint player score record is invalid");
        previous_player=player_id;
        saves::SavedPlayerObservation *found=nullptr;
        for(auto &player:replacement.all_players->players)
            if(player.saved_player_id==player_id) {
                require(!found,"Duplicate BNW map checkpoint source player ID");
                found=&player;
            }
        require(found && land_score(*found)==base,
                "BNW map checkpoint player score source changed");
        set_land_score(*found,live);
    }
    std::uint64_t previous_plot=0,previous_team=0;
    const auto teams=replacement.map_plots->dimensions.teams;
    for(std::uint64_t n=0;n<visibility_count;++n) {
        const auto plot_id=in.u64(),team=in.u64();
        const auto base=in.i64(),live=in.i64();
        require(plot_id<size && team<teams &&
                (n==0 || plot_id>previous_plot ||
                 (plot_id==previous_plot && team>previous_team)),
                "BNW map checkpoint visibility IDs are not unique and ordered");
        previous_plot=plot_id;previous_team=team;
        validate_visibility_change(base,live);
        auto &array=mutable_visibility(
            replacement.map_plots->plots[static_cast<std::size_t>(plot_id)],teams);
        require(array.values[static_cast<std::size_t>(team)]==base,
                "BNW map checkpoint source visibility changed");
        array.values[static_cast<std::size_t>(team)]=live;
    }
    struct YieldChange {
        std::uint64_t index;
        std::array<std::int16_t,6> live;
    };
    require(yield_count<=(section.bytes.size()-in.at)/32,
            "Truncated BNW yield-cache records");
    std::vector<YieldChange> yield_changes;
    yield_changes.reserve(static_cast<std::size_t>(yield_count));
    std::uint64_t previous_yield_plot=0;
    for(std::uint64_t n=0;n<yield_count;++n) {
        const auto index=in.u64();
        require(index<size&&(n==0||index>previous_yield_plot),
                "BNW yield-cache plot IDs are not unique and ordered");previous_yield_plot=index;
        std::array<std::int16_t,6> base{},live{};
        for(auto& value:base)value=in.i16();
        for(auto& value:live)value=in.i16();
        require(base!=live&&base==yield_cache(world,index),"BNW yield-cache before-image differs or unchanged");
        yield_changes.push_back({index,live});
    }
    require(in.at==section.bytes.size(),"Trailing BNW map checkpoint bytes");
    require(live_owned==checked_owned(base_owned,owned_delta,size),
            "BNW map checkpoint owned-plot counter disagrees with owner changes");
    // Validate the complete ordered cache suffix and every before-image before
    // assigning any yield. All writes still target the detached replacement.
    for(const auto& change:yield_changes)
        set_yield_cache(replacement.map_plots->plots.at(
            static_cast<std::size_t>(change.index)),change.live);
    replacement.map_header.owned_plots=live_owned;
    replacement.map_projection=saves::project_saved_map(replacement.map_header,*replacement.map_plots,features);
    if(live_city_mask)
        for(std::size_t i=0;i<live_city_mask->size();++i)
            replacement.map_projection->cells[i].is_city=live_city_mask->at(i);
    replacement.map_terrain=saves::project_saved_map_terrain(*replacement.map_projection);
    for(const auto index:changed_indices)
        validate_changed_types(world.map_plots->plots[static_cast<std::size_t>(index)],
            replacement.map_plots->plots[static_cast<std::size_t>(index)],
            world.map_projection->cells[static_cast<std::size_t>(index)],
            replacement.map_projection->cells[static_cast<std::size_t>(index)],
            features,rules,types,replacement.map_plots->dimensions.players,
            replacement.all_players?&*replacement.all_players:nullptr);
    for(const auto index:changed_indices)
        validate_builder_scratch(
            world.map_plots->plots[static_cast<std::size_t>(index)],
            replacement.map_plots->plots[static_cast<std::size_t>(index)],
            rules,types,replacement.all_players?&*replacement.all_players:nullptr);
    if(version>=3) {
        const auto scores=player_score_changes(world,replacement);
        require(scores.size()==player_count,"BNW map checkpoint player score records mismatch");
        validate_land_scores(world,replacement,scores,defines);
    }
    world=std::move(replacement);
}
} // namespace civ5::runtime

#include "civ5/runtime/bnw_area_unit_checkpoint.hpp"
#include <algorithm>
#include <array>
#include <climits>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','A','U','N','1'};
constexpr std::size_t wire_size=8+4+64+64+8*11;
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {return value?std::string_view(value):std::string_view();}
void check_hash(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW area unit checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i64(std::int64_t value) {u64(static_cast<std::uint64_t>(value));}
    void hash(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW area unit checkpoint");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW area unit checkpoint");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i64() {return static_cast<std::int64_t>(u64());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW area unit checkpoint payload or rules changed");
        at+=expected.size();
    }
};
std::int64_t field(const saves::MapRegionRecord &record,std::string_view wanted) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &candidate:record.fields)
        if(name(candidate.name)==wanted) {
            require(!found,"Duplicate BNW area scalar");found=&candidate;
        }
    require(found,"Missing BNW area scalar");
    return found->value;
}
void set_field(saves::MapRegionRecord &record,std::string_view wanted,std::int64_t value) {
    require(value>=0 && value<=INT32_MAX,"BNW area unit total exceeds i32 wire range");
    (void)field(record,wanted);
    for(auto &candidate:record.fields)
        if(name(candidate.name)==wanted) {candidate.value=value;return;}
}
const saves::PlotArrayObservation &player_units(const saves::MapRegionRecord &record) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:record.arrays)
        if(name(array.name)=="units_per_player") {
            require(!found,"Duplicate BNW area units-per-player array");found=&array;
        }
    require(found && found->columns==1 && !found->values.empty(),
            "Missing BNW area units-per-player array");
    return *found;
}
saves::PlotArrayObservation &mutable_player_units(saves::MapRegionRecord &record) {
    (void)player_units(record);
    for(auto &array:record.arrays)
        if(name(array.name)=="units_per_player") return array;
    throw std::runtime_error("Missing BNW area units-per-player array");
}
std::int64_t plot_area(const saves::SavedSessionSnapshot &world,std::uint64_t index) {
    require(world.map_plots && index<world.map_header.grid.size() &&
            index<world.map_plots->plots.size(),"BNW area transfer plot ID is outside map");
    const auto &plot=world.map_plots->plots[static_cast<std::size_t>(index)];
    require(plot.index==index,"BNW area transfer plot identity changed");
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &candidate:plot.fields)
        if(name(candidate.name)=="area") {
            require(!found,"Duplicate BNW plot area ID");found=&candidate;
        }
    require(found && found->value>=0 && found->value<=INT32_MAX,
            "BNW plot area ID is unavailable");
    return found->value;
}
const saves::MapRegionContainer &areas(const saves::MapRegionsObservation &regions) {
    const saves::MapRegionContainer *found=nullptr;
    for(const auto &container:regions.containers)
        if(name(container.name)=="areas") {
            require(!found,"Duplicate BNW area container");found=&container;
        }
    require(found && found->wire_count>=0 &&
            static_cast<std::uint64_t>(found->wire_count)==found->records.size(),
            "BNW area container is unavailable or malformed");
    return *found;
}
saves::MapRegionContainer &mutable_areas(saves::MapRegionsObservation &regions) {
    (void)areas(regions);
    for(auto &container:regions.containers)
        if(name(container.name)=="areas") return container;
    throw std::runtime_error("Missing BNW area container");
}
std::size_t area_index(const saves::MapRegionContainer &container,std::int64_t id) {
    std::optional<std::size_t> found;
    for(std::size_t i=0;i<container.records.size();++i)
        if(field(container.records[i],"id")==id) {
            require(!found,"Duplicate BNW area ID");found=i;
        }
    require(found.has_value(),"BNW plot area ID is absent from areas");
    return *found;
}
bool same_resources(const std::vector<saves::MapResourceCount> &a,
                    const std::vector<saves::MapResourceCount> &b) {
    if(a.size()!=b.size()) return false;
    for(std::size_t i=0;i<a.size();++i)
        if(a[i].type_hash!=b[i].type_hash || a[i].count!=b[i].count) return false;
    return true;
}
bool same_regions(const saves::MapRegionsObservation &a,
                  const saves::MapRegionsObservation &b) {
    if(a.layout!=b.layout || a.start_offset!=b.start_offset ||
       a.end_offset!=b.end_offset || a.ai_map_hints!=b.ai_map_hints ||
       a.dimensions.has_value()!=b.dimensions.has_value() ||
       a.containers.size()!=b.containers.size()) return false;
    if(a.dimensions && (a.dimensions->fixed_players!=b.dimensions->fixed_players ||
       a.dimensions->fixed_teams!=b.dimensions->fixed_teams ||
       a.dimensions->players!=b.dimensions->players ||
       a.dimensions->yields!=b.dimensions->yields)) return false;
    for(std::size_t c=0;c<a.containers.size();++c) {
        const auto &x=a.containers[c],&y=b.containers[c];
        if(name(x.name)!=name(y.name) || x.start_offset!=y.start_offset ||
           x.end_offset!=y.end_offset || x.wire_count!=y.wire_count ||
           x.legacy_metadata!=y.legacy_metadata || x.next_free!=y.next_free ||
           x.records.size()!=y.records.size()) return false;
        for(std::size_t r=0;r<x.records.size();++r) {
            const auto &p=x.records[r],&q=y.records[r];
            if(p.start_offset!=q.start_offset || p.end_offset!=q.end_offset ||
               p.version!=q.version || p.fields.size()!=q.fields.size() ||
               p.arrays.size()!=q.arrays.size() ||
               p.yield_modifiers.size()!=q.yield_modifiers.size() ||
               !same_resources(p.resources,q.resources) ||
               !same_resources(p.improvements,q.improvements)) return false;
            for(std::size_t i=0;i<p.fields.size();++i)
                if(name(p.fields[i].name)!=name(q.fields[i].name) ||
                   p.fields[i].value!=q.fields[i].value) return false;
            for(std::size_t i=0;i<p.arrays.size();++i)
                if(name(p.arrays[i].name)!=name(q.arrays[i].name) ||
                   p.arrays[i].columns!=q.arrays[i].columns ||
                   p.arrays[i].values!=q.arrays[i].values) return false;
            for(std::size_t i=0;i<p.yield_modifiers.size();++i)
                if(p.yield_modifiers[i].key!=q.yield_modifiers[i].key ||
                   p.yield_modifiers[i].wire_count!=q.yield_modifiers[i].wire_count ||
                   p.yield_modifiers[i].values!=q.yield_modifiers[i].values) return false;
        }
    }
    return true;
}
void same_transfer(const BnwAreaUnitTransfer &a,const BnwAreaUnitTransfer &b) {
    require(a.owner==b.owner && a.from_plot==b.from_plot &&
            a.to_plot==b.to_plot && a.from_area==b.from_area &&
            a.to_area==b.to_area && a.from_units==b.from_units &&
            a.from_owner_units==b.from_owner_units &&
            a.to_units==b.to_units && a.to_owner_units==b.to_owner_units,
            "BNW area transfer source counters or IDs changed");
}
BnwAreaUnitTransfer read_transfer(
    const saves::CivisSaveSection &section,const saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    require(section.id==bnw_area_unit_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()==wire_size &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW area unit checkpoint section");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW area unit checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto width=in.u64(),height=in.u64();
    BnwAreaUnitTransfer encoded;
    encoded.owner=in.i64();encoded.from_plot=in.u64();encoded.to_plot=in.u64();
    encoded.from_area=in.i64();encoded.to_area=in.i64();
    encoded.from_units=in.i64();encoded.from_owner_units=in.i64();
    encoded.to_units=in.i64();encoded.to_owner_units=in.i64();
    require(in.at==section.bytes.size() &&
            width==world.map_header.grid.width &&
            height==world.map_header.grid.height,
            "BNW area checkpoint dimensions or length changed");
    return encoded;
}
const std::vector<std::int64_t> &plot_roster(const saves::PlotObservation &plot) {
    const saves::PlotArrayObservation *found=nullptr;
    for(const auto &array:plot.arrays)
        if(name(array.name)=="units") {
            require(!found,"Duplicate BNW plot unit roster");found=&array;
        }
    require(found && found->columns==2 && found->values.size()%2==0,
            "BNW plot unit roster is unavailable or malformed");
    return found->values;
}
struct RosterTransfer {std::int64_t owner=-1,id=-1;std::uint64_t from=0,to=0;};
RosterTransfer inspect_roster_transfer(const saves::SavedSessionSnapshot &before,
                                       const saves::SavedSessionSnapshot &after) {
    require(before.map_plots && after.map_plots &&
            before.map_plots->plots.size()==after.map_plots->plots.size(),
            "BNW area transfer plot roster dimensions changed");
    std::optional<RosterTransfer> removal,addition;
    for(std::size_t i=0;i<before.map_plots->plots.size();++i) {
        const auto &old=plot_roster(before.map_plots->plots[i]);
        const auto &now=plot_roster(after.map_plots->plots[i]);
        if(old==now) continue;
        if(old.size()==2 && now.empty()) {
            require(!removal,"BNW area transfer has multiple roster removals");
            removal=RosterTransfer{old[0],old[1],i,0};
        } else if(old.empty() && now.size()==2) {
            require(!addition,"BNW area transfer has multiple roster additions");
            addition=RosterTransfer{now[0],now[1],0,i};
        } else throw std::runtime_error("BNW area transfer has unsupported roster change");
    }
    require(removal && addition && removal->owner==addition->owner &&
            removal->id==addition->id && removal->owner>=0 && removal->id>=0,
            "BNW area transfer roster handles do not match");
    return {removal->owner,removal->id,removal->from,addition->to};
}
} // namespace

BnwAreaUnitTransfer plan_bnw_area_unit_transfer(
    const saves::SavedSessionSnapshot &world,std::int64_t owner,
    std::uint64_t from_plot,std::uint64_t to_plot) {
    require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.map_regions && world.map_regions->layout==bnw &&
            world.map_regions->dimensions && world.all_players &&
            world.all_players->layout==bnw,
            "BNW area transfer requires decoded BNW areas and players");
    const auto dimensions=*world.map_regions->dimensions;
    require(dimensions.fixed_players==80 && owner>=0 &&
            static_cast<std::uint64_t>(owner)<dimensions.fixed_players &&
            std::any_of(world.all_players->players.begin(),world.all_players->players.end(),
                [&](const auto &player){return player.saved_player_id==owner;}),
            "BNW area transfer owner is outside active player roster");
    const auto from_area=plot_area(world,from_plot),to_area=plot_area(world,to_plot);
    require(from_area!=to_area,"BNW same-area move has no area-counter checkpoint");
    const auto &container=areas(*world.map_regions);
    const auto &old=container.records[area_index(container,from_area)];
    const auto &next=container.records[area_index(container,to_area)];
    const auto &old_array=player_units(old),&next_array=player_units(next);
    require(old_array.values.size()==dimensions.fixed_players &&
            next_array.values.size()==dimensions.fixed_players,
            "BNW area player-count dimensions changed");
    const auto old_units=field(old,"units"),new_units=field(next,"units");
    const auto old_owner=old_array.values[static_cast<std::size_t>(owner)];
    const auto new_owner=next_array.values[static_cast<std::size_t>(owner)];
    require(old_units>0 && old_units<=INT32_MAX && old_owner>0 &&
            old_owner<=INT32_MAX && new_units>=0 && new_units<INT32_MAX &&
            new_owner>=0 && new_owner<INT32_MAX,
            "BNW area unit transfer underflows or overflows SDK i32 counters");
    return {owner,from_area,to_area,from_plot,to_plot,
            old_units,old_owner,new_units,new_owner};
}

void apply_bnw_area_unit_transfer(
    saves::SavedSessionSnapshot &world,const BnwAreaUnitTransfer &transfer) {
    const auto proven=plan_bnw_area_unit_transfer(
        world,transfer.owner,transfer.from_plot,transfer.to_plot);
    same_transfer(transfer,proven);
    auto replacement=*world.map_regions;
    auto &container=mutable_areas(replacement);
    auto &old=container.records[area_index(container,transfer.from_area)];
    auto &next=container.records[area_index(container,transfer.to_area)];
    // Expansion2 CvUnit::setXY old area decrement precedes coordinate change;
    // the new area increment follows new-plot addUnit. Only these two CvArea
    // fields change in CvArea::changeUnitsPerPlayer (CvArea.cpp:361-370).
    set_field(old,"units",transfer.from_units-1);
    mutable_player_units(old).values[static_cast<std::size_t>(transfer.owner)]=
        transfer.from_owner_units-1;
    set_field(next,"units",transfer.to_units+1);
    mutable_player_units(next).values[static_cast<std::size_t>(transfer.owner)]=
        transfer.to_owner_units+1;
    world.map_regions=std::move(replacement);
}

saves::CivisSaveSection make_bnw_area_unit_checkpoint(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,const BnwAreaUnitTransfer &transfer) {
    check_hash(identity.payload_sha256);check_hash(identity.rules_sha256);
    require(world.map_header.grid.width==source.map_header.grid.width &&
            world.map_header.grid.height==source.map_header.grid.height &&
            plot_area(world,transfer.from_plot)==transfer.from_area &&
            plot_area(world,transfer.to_plot)==transfer.to_area,
            "BNW area checkpoint map dimensions or plot-area IDs changed");
    auto expected=source;
    apply_bnw_area_unit_transfer(expected,transfer);
    require(world.map_regions && expected.map_regions &&
            same_regions(*world.map_regions,*expected.map_regions),
            "BNW area checkpoint has unsupported region mutation");
    Writer out;out.bytes.reserve(wire_size);
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);
    out.hash(identity.payload_sha256);out.hash(identity.rules_sha256);
    out.u64(source.map_header.grid.width);out.u64(source.map_header.grid.height);
    out.i64(transfer.owner);out.u64(transfer.from_plot);out.u64(transfer.to_plot);
    out.i64(transfer.from_area);out.i64(transfer.to_area);
    out.i64(transfer.from_units);out.i64(transfer.from_owner_units);
    out.i64(transfer.to_units);out.i64(transfer.to_owner_units);
    return {bnw_area_unit_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_area_unit_checkpoint(
    const saves::CivisSaveSection &section,saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity) {
    const auto encoded=read_transfer(section,world,identity);
    const auto proven=plan_bnw_area_unit_transfer(
        world,encoded.owner,encoded.from_plot,encoded.to_plot);
    same_transfer(encoded,proven);
    auto replacement=world;
    apply_bnw_area_unit_transfer(replacement,proven);
    world=std::move(replacement);
}

BnwMapUnitAreaCheckpointTriplet make_bnw_map_unit_area_checkpoint_triplet(
    const saves::SavedSessionSnapshot &world,const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,const BnwAreaUnitTransfer &transfer,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules,const content::InfoTypeCatalog *types,
    const content::GlobalDefineCache *defines) {
    auto pair=make_bnw_map_unit_checkpoint_pair(
        world,source,identity,features,rules,types,defines);
    const auto moved=inspect_roster_transfer(source,world);
    require(moved.owner==transfer.owner && moved.from==transfer.from_plot &&
            moved.to==transfer.to_plot,
            "BNW area accounting does not match unit relocation");
    auto area=make_bnw_area_unit_checkpoint(world,source,identity,transfer);
    return {std::move(pair.map),std::move(pair.units),std::move(area)};
}

void restore_bnw_map_unit_area_checkpoint_triplet(
    const saves::CivisSaveSection *map,const saves::CivisSaveSection *units,
    const saves::CivisSaveSection *area,saves::SavedSessionSnapshot &world,
    const SessionCheckpointIdentity &identity,
    const saves::SavedTerrainFeatureResolver &features,
    const content::RulesDatabase *rules,const content::InfoTypeCatalog *types,
    const content::GlobalDefineCache *defines) {
    require(map && units && area,"BNW cross-area relocation requires map, unit and area sections");
    auto replacement=world;
    restore_bnw_map_unit_checkpoint_pair(
        map,units,replacement,identity,features,rules,types,defines);
    const auto moved=inspect_roster_transfer(world,replacement);
    const auto transfer=plan_bnw_area_unit_transfer(
        world,moved.owner,moved.from,moved.to);
    const auto encoded=read_transfer(*area,world,identity);
    require(encoded.owner==moved.owner && encoded.from_plot==moved.from &&
            encoded.to_plot==moved.to,
            "BNW area checkpoint plot IDs do not match unit relocation");
    auto expected=world;
    apply_bnw_area_unit_transfer(expected,transfer);
    restore_bnw_area_unit_checkpoint(*area,replacement,identity);
    require(expected.map_regions && replacement.map_regions &&
            same_regions(*expected.map_regions,*replacement.map_regions),
            "BNW area checkpoint does not account for relocated unit");
    world=std::move(replacement);
}
} // namespace civ5::runtime

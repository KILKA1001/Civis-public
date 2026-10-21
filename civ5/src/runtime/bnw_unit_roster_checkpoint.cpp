#include "civ5/runtime/bnw_unit_roster_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <set>
#include <span>
#include <stdexcept>
#include <string_view>
#include <type_traits>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','U','R','M','1'};
constexpr std::uint64_t index_mask=8191,serial_stride=8192;
constexpr std::uint64_t wire_size=8+4+64*4+7*8;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW unit roster checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t n) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(n>>(8*i)));
    }
    void u64(std::uint64_t n) {
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(n>>(8*i)));
    }
    void i64(std::int64_t n) {u64(std::bit_cast<std::uint64_t>(n));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {u64(value.size());raw(value);}
    void blob(const std::vector<std::uint8_t> &value) {
        u64(value.size());bytes.insert(bytes.end(),value.begin(),value.end());
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW unit removal");
        std::uint32_t n=0;
        for(unsigned i=0;i<4;++i) n|=std::uint32_t(bytes[at++])<<(8*i);
        return n;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW unit removal");
        std::uint64_t n=0;
        for(unsigned i=0;i<8;++i) n|=std::uint64_t(bytes[at++])<<(8*i);
        return n;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW unit removal source, rules or state changed");
        at+=expected.size();
    }
};
template<class Unit> std::int64_t core(const Unit &unit,std::string_view wanted) {
    const saves::UnitCoreField *found=nullptr;
    for(const auto &field:unit.core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW unit core field");found=&field;
        }
    require(found && found->encoding==saves::UnitCoreEncoding::i32 &&
            found->values.size()==1,"BNW unit ID/owner/coordinate unavailable");
    return found->values[0];
}
template<class Header> auto &header_field(Header &header,std::string_view wanted) {
    using Field=std::conditional_t<std::is_const_v<Header>,
        const saves::NamedSaveInteger,saves::NamedSaveInteger>;
    Field *found=nullptr;
    for(auto &field:header.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW unit free-list field");found=&field;
        }
    require(found,"BNW unit free-list field unavailable");
    return found->value;
}
template<class Plot> auto &plot_units(Plot &plot) {
    using Array=std::conditional_t<std::is_const_v<Plot>,
        const saves::PlotArrayObservation,saves::PlotArrayObservation>;
    Array *found=nullptr;
    for(auto &array:plot.arrays)
        if(name(array.name)=="units") {
            require(!found,"Duplicate BNW plot unit roster");found=&array;
        }
    require(found && found->columns==2 && found->values.size()%2==0,
            "BNW plot unit roster unavailable");
    return found->values;
}
template<class Record> auto &area_field(Record &record,std::string_view wanted) {
    using Field=std::conditional_t<std::is_const_v<Record>,
        const saves::NamedSaveInteger,saves::NamedSaveInteger>;
    Field *found=nullptr;
    for(auto &field:record.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW area field");found=&field;
        }
    require(found,"BNW area field unavailable");return found->value;
}
template<class Record> auto &area_player_units(Record &record) {
    using Array=std::conditional_t<std::is_const_v<Record>,
        const saves::PlotArrayObservation,saves::PlotArrayObservation>;
    Array *found=nullptr;
    for(auto &array:record.arrays)
        if(name(array.name)=="units_per_player") {
            require(!found,"Duplicate BNW area owner counts");found=&array;
        }
    require(found && found->columns==1 && !found->values.empty(),
            "BNW area owner counts unavailable");return found->values;
}
template<class World> auto &area_container(World &world) {
    require(world.map_regions && world.map_regions->layout==bnw,
            "BNW unit removal needs decoded areas");
    using Container=std::conditional_t<std::is_const_v<World>,
        const saves::MapRegionContainer,saves::MapRegionContainer>;
    Container *found=nullptr;
    for(auto &container:world.map_regions->containers)
        if(name(container.name)=="areas") {
            require(!found,"Duplicate BNW area container");found=&container;
        }
    require(found && found->wire_count>=0 &&
            static_cast<std::uint64_t>(found->wire_count)==found->records.size(),
            "BNW area container unavailable or malformed");
    return *found;
}
std::int64_t plot_area(const saves::PlotObservation &plot) {
    const saves::NamedSaveInteger *found=nullptr;
    for(const auto &field:plot.fields)
        if(name(field.name)=="area") {
            require(!found,"Duplicate BNW plot area ID");found=&field;
        }
    require(found && found->value>=INT32_MIN && found->value<=INT32_MAX,
            "BNW plot area unavailable");return found->value;
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.map_header.layout==bnw &&
            world.map_plots && world.map_plots->layout==bnw &&
            world.map_projection && world.map_regions &&
            world.all_players && world.all_players->layout==bnw &&
            world.all_players->players.size()==64 &&
            world.map_plots->plots.size()==world.map_header.grid.size() &&
            world.map_projection->cells.size()==world.map_header.grid.size(),
            "BNW unit removal needs complete decoded players/map/areas");
}
void validate_header(const saves::UnitsObservation &units) {
    const auto &h=units.header;
    require(units.layout==bnw && h.layout==bnw && h.fields.size()==5 &&
            name(h.fields[0].name)=="num_slots" &&
            name(h.fields[1].name)=="last_index" &&
            name(h.fields[2].name)=="free_list_head" &&
            name(h.fields[3].name)=="free_list_count" &&
            name(h.fields[4].name)=="current_id" &&
            h.wire_unit_count>=0 &&
            static_cast<std::uint64_t>(h.wire_unit_count)==units.units.size(),
            "BNW unit free-list header or count changed");
    const auto slots=h.fields[0].value,last=h.fields[1].value;
    const auto head=h.fields[2].value,free_count=h.fields[3].value;
    const auto serial=h.fields[4].value;
    require(slots>=0 && slots<=8192 &&
            h.source_slots==static_cast<std::uint64_t>(slots) &&
            h.next_free_indices.size()==h.source_slots &&
            last>=-1 && last<slots && free_count>=0 &&
            free_count<=last+1 && serial>=static_cast<std::int64_t>(serial_stride) &&
            serial%static_cast<std::int64_t>(serial_stride)==0 &&
            ((free_count==0 && head==-1) ||
             (free_count>0 && head>=0 && head<=last)),
            "BNW unit free-list dimensions or serial invalid");
    std::set<std::int64_t> occupied,freed;
    std::int64_t previous=-1;
    for(std::size_t i=0;i<units.units.size();++i) {
        const auto &unit=units.units[i];
        const auto id=core(unit,"m_iID");
        const auto slot=id&static_cast<std::int64_t>(index_mask);
        require(unit.index==i && id>=0 && slot>previous && slot<=last &&
                (id&~static_cast<std::int64_t>(index_mask))<serial &&
                h.next_free_indices[static_cast<std::size_t>(slot)]==-1 &&
                occupied.insert(slot).second,
                "BNW unit roster is not the SDK slot order");
        previous=slot;
    }
    auto next=head;
    for(std::int64_t i=0;i<free_count;++i) {
        require(next>=0 && next<=last && !occupied.contains(next) &&
                freed.insert(next).second,"BNW unit free-list chain invalid");
        next=h.next_free_indices[static_cast<std::size_t>(next)];
    }
    require(next==-1 && occupied.size()+freed.size()==
            static_cast<std::size_t>(last+1),
            "BNW unit free-list does not partition occupied slots");
}
void write_tail(Writer &out,const saves::UnitTailNode &node,
                std::size_t depth=0) {
    require(depth<24,"BNW unit tail source nesting changed");
    out.string(name(node.name));out.string(name(node.encoding));
    out.u64(node.start_offset);out.u64(node.end_offset);
    out.u32(node.wire_count.has_value()?1:0);
    if(node.wire_count) out.i64(*node.wire_count);
    out.u64(node.values.size());for(const auto value:node.values) out.i64(value);
    out.blob(node.bytes);out.u64(node.children.size());
    for(const auto &child:node.children) write_tail(out,child,depth+1);
}
void write_unit(Writer &out,const saves::SavedUnitObservation &unit) {
    out.u64(unit.index);out.u32(static_cast<std::uint32_t>(unit.core.layout));
    out.u64(unit.core.start_offset);out.u64(unit.core.end_offset);
    out.u32(unit.core.version.has_value()?1:0);
    if(unit.core.version) out.u64(*unit.core.version);
    out.u64(unit.core.fields.size());
    for(const auto &field:unit.core.fields) {
        out.string(name(field.name));
        out.u32(static_cast<std::uint32_t>(field.encoding));
        out.u64(field.start_offset);out.u64(field.end_offset);
        out.u32(field.wire_count.has_value()?1:0);
        if(field.wire_count) out.u64(*field.wire_count);
        out.u64(field.values.size());
        for(const auto value:field.values) out.i64(value);
        out.blob(field.bytes);
    }
    out.u32(static_cast<std::uint32_t>(unit.tail.layout));
    out.u32(unit.tail.context.bnw_unit_version.has_value()?1:0);
    if(unit.tail.context.bnw_unit_version)
        out.u64(*unit.tail.context.bnw_unit_version);
    out.u64(unit.tail.start_offset);out.u64(unit.tail.end_offset);
    out.u64(unit.tail.blocks.size());
    for(const auto &block:unit.tail.blocks) write_tail(out,block);
}
std::string state_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    out.u64(world.map_header.grid.width);out.u64(world.map_header.grid.height);
    for(const auto &player:world.all_players->players) {
        const auto &units=player.units;
        out.u64(player.index);out.i64(player.saved_player_id);
        out.u32(static_cast<std::uint32_t>(units.layout));
        out.u64(units.start_offset);out.u64(units.end_offset);
        out.u64(units.retained_elements);out.u64(units.retained_bytes);
        out.u32(static_cast<std::uint32_t>(units.header.layout));
        out.u64(units.header.start_offset);out.u64(units.header.end_offset);
        out.u64(units.header.source_slots);
        out.u64(units.header.fields.size());
        for(const auto &field:units.header.fields) {
            out.string(name(field.name));out.i64(field.value);
        }
        out.u64(units.header.next_free_indices.size());
        for(const auto n:units.header.next_free_indices) out.i64(n);
        out.i64(units.header.wire_unit_count);out.u64(units.units.size());
        for(const auto &unit:units.units) write_unit(out,unit);
    }
    for(std::size_t i=0;i<world.map_plots->plots.size();++i) {
        const auto &plot=world.map_plots->plots[i];
        const auto &cell=world.map_projection->cells[i];
        require(plot.index==i && cell.index==i && cell.units,
                "BNW unit removal plot projection unavailable");
        const auto xy=world.map_header.grid.coordinates(i);
        require(plot.x==xy[0] && plot.y==xy[1] &&
                cell.x==xy[0] && cell.y==xy[1],
                "BNW unit removal plot coordinates changed");
        const auto &raw=plot_units(plot);
        require(raw.size()==cell.units->size()*2,
                "BNW unit removal raw/projected plot roster differs");
        out.u64(i);out.i64(plot_area(plot));out.u64(raw.size());
        for(std::size_t j=0;j<cell.units->size();++j) {
            require(raw[2*j]==(*cell.units)[j].owner &&
                    raw[2*j+1]==(*cell.units)[j].id,
                    "BNW unit removal raw/projected plot handle differs");
            out.i64(raw[2*j]);out.i64(raw[2*j+1]);
        }
    }
    const auto &areas=area_container(world);
    out.string(name(areas.name));out.u64(areas.start_offset);
    out.u64(areas.end_offset);out.i64(areas.wire_count);
    out.u64(areas.legacy_metadata.size());
    for(const auto value:areas.legacy_metadata) out.i64(value);
    out.u64(areas.next_free.size());
    for(const auto value:areas.next_free) out.i64(value);
    out.u64(areas.records.size());
    for(const auto &area:areas.records) {
        out.u64(area.start_offset);out.u64(area.end_offset);
        out.u32(area.version.has_value()?1:0);
        if(area.version) out.u64(*area.version);
        out.u64(area.fields.size());
        for(const auto &field:area.fields) {
            out.string(name(field.name));out.i64(field.value);
        }
        out.u64(area.arrays.size());
        for(const auto &array:area.arrays) {
            out.string(name(array.name));out.u64(array.columns);
            out.u64(array.values.size());
            for(const auto value:array.values) out.i64(value);
        }
        out.u64(area.yield_modifiers.size());
        for(const auto &row:area.yield_modifiers) {
            out.i64(row.key);out.u64(row.wire_count);
            out.u64(row.values.size());
            for(const auto value:row.values) out.i64(value);
        }
        for(const auto *counts:{&area.resources,&area.improvements}) {
            out.u64(counts->size());
            for(const auto &entry:*counts) {
                out.u32(entry.type_hash.has_value()?1:0);
                if(entry.type_hash) out.u64(*entry.type_hash);
                out.u32(entry.count.has_value()?1:0);
                if(entry.count) out.i64(*entry.count);
            }
        }
    }
    return identify_bytes(out.bytes).sha256;
}
void same_removal(const BnwUnitRemoval &a,const BnwUnitRemoval &b) {
    require(a.player_index==b.player_index && a.unit_index==b.unit_index &&
            a.plot_index==b.plot_index && a.area_index==b.area_index &&
            a.owner==b.owner && a.id==b.id && a.area_id==b.area_id,
            "BNW unit removal source object or map/area reference changed");
}
} // namespace

BnwUnitRemoval plan_bnw_unit_removal(
    const saves::SavedSessionSnapshot &source,
    std::uint64_t player_index,std::uint64_t unit_index) {
    shape(source);
    require(player_index<source.all_players->players.size(),
            "BNW unit removal player outside roster");
    const auto &player=source.all_players->players[player_index];
    require(player.index==player_index &&
            player.saved_player_id==static_cast<std::int64_t>(player_index),
            "BNW unit removal player ID mismatch");
    validate_header(player.units);
    require(unit_index<player.units.units.size(),"BNW unit removal unit outside roster");
    const auto &unit=player.units.units[unit_index];
    const auto owner=core(unit,"m_eOwner"),id=core(unit,"m_iID");
    const auto x=core(unit,"m_iX"),y=core(unit,"m_iY");
    require(owner==player.saved_player_id && id>=0 && x>=0 && y>=0 &&
            static_cast<std::uint64_t>(x)<source.map_header.grid.width &&
            static_cast<std::uint64_t>(y)<source.map_header.grid.height,
            "BNW unit removal owner or plot unavailable");
    const auto plot_index=source.map_header.grid.index(x,y);
    std::size_t occurrences=0;
    for(const auto &plot:source.map_plots->plots) {
        const auto &roster=plot_units(plot);
        for(std::size_t j=0;j<roster.size();j+=2)
            if(roster[j]==owner && roster[j+1]==id) {
                require(plot.index==plot_index,
                        "BNW unit removal handle occurs on another plot");
                ++occurrences;
            }
    }
    require(occurrences==1,"BNW unit removal handle is absent or duplicated");
    const auto area_id=plot_area(source.map_plots->plots[plot_index]);
    require(area_id>=0,"BNW unit removal plot has no area");
    const auto &areas=area_container(source);
    std::optional<std::size_t> area_index;
    for(std::size_t i=0;i<areas.records.size();++i)
        if(area_field(areas.records[i],"id")==area_id) {
            require(!area_index,"Duplicate BNW area ID");area_index=i;
        }
    require(area_index.has_value(),"BNW unit removal plot area missing");
    const auto &area=areas.records[*area_index];
    const auto &owner_counts=area_player_units(area);
    require(static_cast<std::uint64_t>(owner)<owner_counts.size() &&
            area_field(area,"units")>0 && owner_counts[owner]>0,
            "BNW unit removal area counts cannot decrement");
    return {player_index,unit_index,plot_index,*area_index,owner,id,area_id};
}

saves::SavedSessionSnapshot project_bnw_unit_removal(
    const saves::SavedSessionSnapshot &source,const BnwUnitRemoval &removal) {
    same_removal(removal,plan_bnw_unit_removal(
        source,removal.player_index,removal.unit_index));
    auto result=source;
    auto &units=result.all_players->players[removal.player_index].units;
    const auto slot=static_cast<std::size_t>(removal.id&static_cast<std::int64_t>(index_mask));
    const auto old_head=header_field(units.header,"free_list_head");
    units.header.next_free_indices[slot]=old_head;
    header_field(units.header,"free_list_head")=static_cast<std::int64_t>(slot);
    ++header_field(units.header,"free_list_count");
    --units.header.wire_unit_count;
    units.units.erase(units.units.begin()+removal.unit_index);
    for(std::size_t i=removal.unit_index;i<units.units.size();++i)
        units.units[i].index=i;
    auto &plot=result.map_plots->plots[removal.plot_index];
    auto &raw=plot_units(plot);
    std::size_t found=raw.size();
    for(std::size_t j=0;j<raw.size();j+=2)
        if(raw[j]==removal.owner && raw[j+1]==removal.id) found=j;
    require(found<raw.size(),"BNW unit removal plot handle disappeared");
    raw.erase(raw.begin()+found,raw.begin()+found+2);
    auto &projected=*result.map_projection->cells[removal.plot_index].units;
    projected.erase(projected.begin()+found/2);
    auto &area=area_container(result).records[removal.area_index];
    --area_field(area,"units");
    --area_player_units(area)[static_cast<std::size_t>(removal.owner)];
    validate_header(units);
    return result;
}

saves::CivisSaveSection make_bnw_unit_roster_removal_checkpoint(
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    const BnwUnitRemoval &removal) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    const auto before=state_hash(source);
    const auto after=state_hash(project_bnw_unit_removal(source,removal));
    Writer out;out.bytes.reserve(wire_size);
    out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(before);out.raw(after);
    out.u64(removal.player_index);out.u64(removal.unit_index);
    out.u64(removal.plot_index);out.u64(removal.area_index);
    out.i64(removal.owner);out.i64(removal.id);out.i64(removal.area_id);
    require(out.bytes.size()==wire_size,"BNW unit removal wire size changed");
    return {bnw_unit_roster_checkpoint_section_id,0,std::move(out.bytes)};
}

saves::SavedSessionSnapshot restore_bnw_unit_roster_removal_checkpoint(
    const saves::CivisSaveSection &section,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_unit_roster_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()==wire_size &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW unit roster removal checkpoint");
    Reader in{section.bytes,magic.size()};
    require(in.u32()==1,"Unsupported BNW unit removal version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    in.hash(state_hash(source));
    const std::string expected_after(reinterpret_cast<const char*>(
        section.bytes.data()+in.at),64);
    in.at+=64;
    BnwUnitRemoval removal;
    removal.player_index=in.u64();removal.unit_index=in.u64();
    removal.plot_index=in.u64();removal.area_index=in.u64();
    removal.owner=in.i64();removal.id=in.i64();removal.area_id=in.i64();
    require(in.at==section.bytes.size(),"BNW unit removal has trailing bytes");
    auto result=project_bnw_unit_removal(source,removal);
    require(state_hash(result)==expected_after,
            "BNW unit removal result state changed");
    return result;
}

void reject_bnw_unit_removal_companion_overlap(
    const std::vector<saves::CivisSaveSection> &sections) {
    bool removal=false,overlap=false;
    std::size_t removal_count=0;
    for(const auto &section:sections) {
        if(section.id==bnw_unit_roster_checkpoint_section_id) {
            removal=true;++removal_count;
        }
        if(section.id==17 || section.id==18 || section.id==19) overlap=true;
    }
    require(removal_count<=1 && !(removal && overlap),
            "BNW unit removal overlaps map/relocation/area checkpoint");
}
} // namespace civ5::runtime

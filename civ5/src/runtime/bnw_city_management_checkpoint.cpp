#include "civ5/runtime/bnw_city_management_checkpoint.hpp"
#include "civ5/runtime/file_identity.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <climits>
#include <span>
#include <stdexcept>
#include <string_view>

namespace civ5::runtime {
namespace {
constexpr auto bnw=saves::GameStateLayout::expansion2_version1;
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','C','M','G','1'};
constexpr std::array<std::string_view,6> blocks{
    "strategy","building_ai","unit_ai","project_ai","process_ai","citizens"};
constexpr std::size_t players=64,max_values=1'000'000;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW city management checkpoint SHA-256");
}
struct Writer {
    std::vector<std::uint8_t> bytes;
    void u32(std::uint32_t value) {
        for(unsigned i=0;i<4;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void u64(std::uint64_t value) {
        for(unsigned i=0;i<8;++i) bytes.push_back(static_cast<std::uint8_t>(value>>(8*i)));
    }
    void i32(std::int64_t value) {
        require(value>=INT32_MIN && value<=INT32_MAX,"BNW city management object ID exceeds i32");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void i64(std::int64_t value) {u64(std::bit_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {
        require(!value.empty() && value.size()<=UINT32_MAX,
                "BNW city management schema name empty or too long");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW management section");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW management section");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW management source, rules, roster or schema changed");
        at+=expected.size();
    }
};
std::int64_t scalar(const saves::CityCoreObservation &core,std::string_view wanted) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW management city identity");found=&field;
        }
    require(found && found->encoding==saves::CityCoreEncoding::i32 &&
            found->values.size()==1,"BNW management city identity unavailable");
    return found->values[0];
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW management needs complete plain player roster");
}
std::string roster_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(std::size_t p=0;p<players;++p) {
        const auto &player=world.all_players->players[p];
        const auto &cities=player.cities;
        require(player.index==p && player.saved_player_id==static_cast<std::int64_t>(p) &&
                cities.layout==bnw && cities.header.layout==bnw &&
                cities.context.profile_extensions.values.empty() &&
                cities.header.wire_city_count>=0 &&
                static_cast<std::uint64_t>(cities.header.wire_city_count)==
                    cities.cities.size(),
                "BNW management player or city count changed");
        out.u32(static_cast<std::uint32_t>(p));out.u64(cities.cities.size());
        for(std::size_t i=0;i<cities.cities.size();++i) {
            const auto &city=cities.cities[i];
            require(city.index==i && city.core.layout==bnw &&
                    scalar(city.core,"m_eOwner")==static_cast<std::int64_t>(p),
                    "BNW management city owner or order changed");
            out.i32(scalar(city.core,"m_iID"));
            out.i32(scalar(city.core,"m_eOwner"));
            out.i32(scalar(city.core,"m_iX"));out.i32(scalar(city.core,"m_iY"));
        }
    }
    return identify_bytes(out.bytes).sha256;
}
std::string schema_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(const auto &player:world.all_players->players)
        for(const auto &city:player.cities.cities) {
            const auto &management=city.management;
            require(management.layout==bnw &&
                    management.context.profile_extensions.values.empty() &&
                    management.context.bnw_projects &&
                    management.city_plot_slots==37 &&
                    management.blocks.size()==blocks.size(),
                    "Unknown BNW city management profile or dimensions");
            out.u64(management.start_offset);out.u64(management.end_offset);
            out.u64(*management.context.bnw_projects);
            out.u64(management.city_plot_slots);
            for(std::size_t i=0;i<blocks.size();++i) {
                const auto &block=management.blocks[i];
                require(name(block.name)==blocks[i],
                        "Unknown BNW city management block/order");
                out.string(name(block.name));
                out.u64(block.start_offset);out.u64(block.end_offset);
                out.u64(block.fields.size());
                for(const auto &field:block.fields) out.string(name(field.name));
                out.u64(block.arrays.size());
                for(const auto &array:block.arrays) {
                    out.string(name(array.name));out.u64(array.columns);
                    out.u64(array.values.size());
                    require(array.columns && array.values.size()%array.columns==0,
                            "BNW management array row shape changed");
                }
                out.u64(block.hashes.size());
                for(const auto &hash:block.hashes) {
                    out.string(name(hash.name));out.i32(hash.wire_count);
                    out.u64(hash.rows.size());
                    require((hash.wire_count>0?
                                 static_cast<std::uint64_t>(hash.wire_count):0)==
                                hash.rows.size(),
                            "BNW management hashed row count changed");
                    for(const auto &row:hash.rows) {
                        require(row.type_hash<=UINT32_MAX && row.values.size()<=1,
                                "Unknown BNW management hashed row shape");
                        out.u64(row.type_hash);out.u64(row.values.size());
                    }
                }
            }
        }
    return identify_bytes(out.bytes).sha256;
}
enum class WireKind {i32,u32,boolean};
WireKind field_kind(std::string_view field) {
    if(field=="version") return WireKind::u32;
    if(field=="m_bAutomated" || field=="m_bNoAutoAssignSpecialists" ||
       field=="m_bForceAvoidGrowth") return WireKind::boolean;
    return WireKind::i32;
}
WireKind array_kind(std::string_view field) {
    if(field=="m_pabWorkingPlot" || field=="m_pabForcedWorkingPlot")
        return WireKind::boolean;
    return WireKind::i32;
}
WireKind hash_kind(std::string_view field) {
    return field=="m_pabUsingCityStrategy"?WireKind::boolean:WireKind::i32;
}
void value_shape(WireKind kind,std::int64_t value) {
    switch(kind) {
    case WireKind::i32:
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW management i32 exceeds wire range");break;
    case WireKind::u32:
        require(value>=0 && value<=UINT32_MAX,
                "BNW management u32 exceeds wire range");break;
    case WireKind::boolean:
        require(value==0 || value==1,"BNW management boolean invalid");break;
    }
}
template<class World,class Fn> void walk_values(World &world,Fn &&visit) {
    for(auto &player:world.all_players->players)
        for(auto &city:player.cities.cities)
            for(auto &block:city.management.blocks) {
                for(auto &field:block.fields)
                    visit(field.name,field_kind(name(field.name)),field.value);
                for(auto &array:block.arrays)
                    for(auto &value:array.values)
                        visit(array.name,array_kind(name(array.name)),value);
                for(auto &hash:block.hashes)
                    for(auto &row:hash.rows)
                        for(auto &value:row.values)
                            visit(hash.name,hash_kind(name(hash.name)),value);
            }
}
bool fixed_field(std::string_view field) {
    return field=="version" || field.ends_with("_wire_count");
}
void write_node(Writer &out,const saves::CityTailNode &node) {
    out.string(name(node.name));out.string(name(node.encoding));
    out.u64(node.start_offset);out.u64(node.end_offset);
    out.u32(node.wire_count.has_value()?1:0);
    if(node.wire_count) out.i64(*node.wire_count);
    out.u64(node.values.size());
    for(const auto value:node.values) out.i64(value);
    out.u64(node.children.size());
    for(const auto &child:node.children) write_node(out,child);
}
std::string other_city_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(const auto &player:world.all_players->players) {
        const auto &cities=player.cities;
        out.u64(cities.start_offset);out.u64(cities.end_offset);
        out.u32(cities.context.bnw_projects.has_value()?1:0);
        if(cities.context.bnw_projects) out.u64(*cities.context.bnw_projects);
        out.u64(cities.retained_elements);out.u64(cities.retained_string_bytes);
        out.u64(cities.header.start_offset);out.u64(cities.header.end_offset);
        out.u64(cities.header.source_slots);out.i64(cities.header.wire_city_count);
        out.u64(cities.header.fields.size());
        for(const auto &field:cities.header.fields) {
            out.string(name(field.name));out.i64(field.value);
        }
        out.u64(cities.header.next_free_indices.size());
        for(const auto value:cities.header.next_free_indices) out.i64(value);
        for(const auto &city:cities.cities) {
            const auto &tail=city.tail;
            require(tail.layout==bnw &&
                    tail.context.profile_extensions.values.empty(),
                    "Unknown BNW management tail profile");
            out.u64(tail.start_offset);out.u64(tail.end_offset);
            out.u32(tail.context.bnw_city_version.has_value()?1:0);
            if(tail.context.bnw_city_version)
                out.u64(*tail.context.bnw_city_version);
            out.u64(tail.blocks.size());
            for(const auto &node:tail.blocks) write_node(out,node);
        }
    }
    return identify_bytes(out.bytes).sha256;
}
} // namespace

saves::CivisSaveSection make_bnw_city_management_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    const auto companions=other_city_hash(source);
    require(roster_hash(world)==roster && schema_hash(world)==schema &&
            other_city_hash(world)==companions,
            "BNW management roster, structure or tail/header changed");
    std::vector<std::int64_t> baseline;
    std::vector<bool> immutable;
    walk_values(source,[&](const char *field,WireKind kind,const auto &value){
        require(baseline.size()<max_values,"BNW management value budget exceeded");
        value_shape(kind,value);baseline.push_back(value);
        immutable.push_back(fixed_field(name(field)));
    });
    Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(roster);out.raw(schema);out.raw(companions);
    out.u32(players);out.u64(baseline.size());
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const char *,WireKind kind,const auto &value){
        require(index<baseline.size(),"BNW management value count changed");
        value_shape(kind,value);
        require(!immutable[index] || value==baseline[index],
                "BNW management version or collection count changed");
        changed|=value!=baseline[index++];out.i64(value);
    });
    require(index==baseline.size() && (changed || allow_unchanged),
            "BNW management checkpoint unchanged or incomplete");
    if(!changed) out.bytes[8]=2;
    return {bnw_city_management_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_city_management_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_city_management_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*5+4+8 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW management checkpoint section");
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require(version==1 || version==2,
            "Unsupported BNW management checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    const auto companions=other_city_hash(source);
    in.hash(roster);in.hash(schema);in.hash(companions);
    require(in.u32()==players && roster_hash(world)==roster &&
            schema_hash(world)==schema && other_city_hash(world)==companions,
            "BNW management destination roster, structure or tail/header changed");
    const auto count=in.u64();
    require(count<=max_values && count<=(section.bytes.size()-in.at)/8 &&
            section.bytes.size()-in.at==count*8,
            "BNW management section count or length changed");
    std::vector<std::int64_t> baseline;
    baseline.reserve(static_cast<std::size_t>(count));
    std::vector<bool> immutable;
    immutable.reserve(static_cast<std::size_t>(count));
    walk_values(source,[&](const char *field,WireKind kind,const auto &value){
        require(baseline.size()<max_values,"BNW management value budget exceeded");
        value_shape(kind,value);baseline.push_back(value);
        immutable.push_back(fixed_field(name(field)));
    });
    require(baseline.size()==count,"BNW management source count changed");
    std::vector<std::pair<std::int64_t*,std::int64_t>> pending;
    pending.reserve(static_cast<std::size_t>(count));
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const char *,WireKind kind,auto &value){
        require(index<baseline.size() && value==baseline[index],
                "BNW management destination is not source baseline");
        const auto next=in.i64();value_shape(kind,next);
        require(!immutable[index] || next==baseline[index],
                "BNW management version or collection count changed");
        changed|=next!=baseline[index++];pending.emplace_back(&value,next);
    });
    require(index==count && changed==(version==1) &&
            in.at==section.bytes.size(),
            "BNW management section incomplete or unchanged");
    for(const auto &[target,value]:pending) *target=value;
}

BnwCityCoreProductionManagementCheckpointTriple
make_bnw_city_core_production_management_checkpoint_triple(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    require(roster_hash(world)==roster_hash(source),
            "BNW city triple roster changed");
    auto core_only=source,production_only=source;
    for(std::size_t p=0;p<players;++p) {
        const auto &from=world.all_players->players[p].cities.cities;
        auto &to_core=core_only.all_players->players[p].cities.cities;
        auto &to_production=production_only.all_players->players[p].cities.cities;
        require(from.size()==to_core.size() && from.size()==to_production.size(),
                "BNW city triple city count changed");
        for(std::size_t i=0;i<from.size();++i) {
            to_core[i].core=from[i].core;
            to_production[i].production=from[i].production;
        }
    }
    BnwCityCoreProductionManagementCheckpointTriple triple;
    triple.core=make_bnw_city_core_checkpoint(core_only,source,identity);
    triple.production=make_bnw_city_production_checkpoint(
        production_only,source,identity);
    triple.management=make_bnw_city_management_checkpoint(world,source,identity);
    return triple;
}

saves::SavedSessionSnapshot restore_bnw_city_core_production_management_checkpoint_triple(
    const BnwCityCoreProductionManagementCheckpointTriple &triple,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    auto replacement=source;
    restore_bnw_city_core_checkpoint(triple.core,replacement,source,identity);
    restore_bnw_city_production_checkpoint(triple.production,replacement,source,identity);
    restore_bnw_city_management_checkpoint(triple.management,replacement,source,identity);
    return replacement;
}
} // namespace civ5::runtime

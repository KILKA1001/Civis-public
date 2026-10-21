#include "civ5/runtime/bnw_city_tail_checkpoint.hpp"
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
constexpr std::array<std::uint8_t,8> magic{'C','I','V','B','C','T','L','1'};
constexpr std::array<std::string_view,5> blocks{
    "religions","emphases","espionage","remainder","city_ai"};
constexpr std::size_t players=64,max_values=1'000'000;
void require(bool ok,const char *why) {if(!ok) throw std::runtime_error(why);}
std::string_view name(const char *value) {
    return value?std::string_view(value):std::string_view();
}
void hash_shape(std::string_view hash) {
    require(hash.size()==64 && std::all_of(hash.begin(),hash.end(),[](char c){
        return (c>='0'&&c<='9')||(c>='a'&&c<='f');
    }),"Invalid BNW city tail checkpoint SHA-256");
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
        require(value>=INT32_MIN && value<=INT32_MAX,"BNW city tail object ID exceeds i32");
        u32(static_cast<std::uint32_t>(static_cast<std::int32_t>(value)));
    }
    void i64(std::int64_t value) {u64(std::bit_cast<std::uint64_t>(value));}
    void raw(std::string_view value) {bytes.insert(bytes.end(),value.begin(),value.end());}
    void string(std::string_view value) {
        require(!value.empty() && value.size()<=UINT32_MAX,
                "BNW city tail schema name empty or too long");
        u32(static_cast<std::uint32_t>(value.size()));raw(value);
    }
};
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t at=0;
    std::uint32_t u32() {
        require(at<=bytes.size() && bytes.size()-at>=4,"Truncated BNW city tail section");
        std::uint32_t value=0;
        for(unsigned i=0;i<4;++i) value|=std::uint32_t(bytes[at++])<<(8*i);
        return value;
    }
    std::uint64_t u64() {
        require(at<=bytes.size() && bytes.size()-at>=8,"Truncated BNW city tail section");
        std::uint64_t value=0;
        for(unsigned i=0;i<8;++i) value|=std::uint64_t(bytes[at++])<<(8*i);
        return value;
    }
    std::int64_t i64() {return std::bit_cast<std::int64_t>(u64());}
    void hash(std::string_view expected) {
        require(at<=bytes.size() && bytes.size()-at>=expected.size() &&
                std::equal(expected.begin(),expected.end(),bytes.begin()+at),
                "BNW city tail source, rules, roster or schema changed");
        at+=expected.size();
    }
};
std::int64_t scalar(const saves::CityCoreObservation &core,std::string_view wanted) {
    const saves::CityCoreField *found=nullptr;
    for(const auto &field:core.fields)
        if(name(field.name)==wanted) {
            require(!found,"Duplicate BNW city tail identity");found=&field;
        }
    require(found && found->encoding==saves::CityCoreEncoding::i32 &&
            found->values.size()==1,"BNW city tail identity unavailable");
    return found->values[0];
}
void shape(const saves::SavedSessionSnapshot &world) {
    require(world.game_state.layout==bnw && world.all_players &&
            world.all_players->layout==bnw &&
            world.all_players->context.player_count==players &&
            world.all_players->players.size()==players &&
            world.all_players->context.profile_extensions.values.empty(),
            "BNW city tail needs complete plain player roster");
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
                "BNW city tail player or city count changed");
        out.u32(static_cast<std::uint32_t>(p));out.u64(cities.cities.size());
        for(std::size_t i=0;i<cities.cities.size();++i) {
            const auto &city=cities.cities[i];
            require(city.index==i && city.core.layout==bnw &&
                    scalar(city.core,"m_eOwner")==static_cast<std::int64_t>(p),
                    "BNW city tail owner or city order changed");
            out.i32(scalar(city.core,"m_iID"));
            out.i32(scalar(city.core,"m_eOwner"));
            out.i32(scalar(city.core,"m_iX"));out.i32(scalar(city.core,"m_iY"));
        }
    }
    return identify_bytes(out.bytes).sha256;
}
void node_schema(Writer &out,const saves::CityTailNode &node,
                 std::size_t depth=0) {
    require(depth<24,"BNW city tail tree exceeds SDK nesting");
    const auto encoding=name(node.encoding);
    require(encoding=="record" || encoding=="i32" || encoding=="u32" ||
            encoding=="bool" || encoding=="span_i32" ||
            encoding=="span_bool" || encoding=="counted_i32" ||
            encoding=="counted_bool" || encoding=="signed_records" ||
            encoding=="stl_records" || encoding=="hashed_bool" ||
            encoding=="stl_pairs_i32" || encoding=="stl_triples_i32",
            "Unknown BNW city tail node encoding");
    out.string(name(node.name));out.string(encoding);
    out.u64(node.start_offset);out.u64(node.end_offset);
    out.u32(node.wire_count.has_value()?1:0);
    if(node.wire_count) {
        require(*node.wire_count>=INT32_MIN && *node.wire_count<=UINT32_MAX,
                "BNW city tail wire count exceeds source range");
        out.i64(*node.wire_count);
    }
    out.u64(node.values.size());out.u64(node.children.size());
    require(node.values.size()<=max_values,
            "BNW city tail node value budget exceeded");
    if(encoding=="record")
        require(node.values.empty() && !node.wire_count,
                "BNW city tail record shape changed");
    if(encoding=="bool" || encoding=="u32" || encoding=="i32")
        require(node.values.size()==1 && node.children.empty() &&
                !node.wire_count,"BNW city tail scalar shape changed");
    if(encoding=="span_bool" || encoding=="span_i32")
        require(!node.wire_count && node.children.empty(),
                "BNW city tail fixed span shape changed");
    if(encoding=="counted_bool" || encoding=="counted_i32" ||
       encoding=="stl_pairs_i32" || encoding=="stl_triples_i32") {
        const auto columns=encoding=="stl_triples_i32"?3u:
                           encoding=="stl_pairs_i32"?2u:1u;
        require(node.wire_count && node.children.empty() &&
                (node.wire_count.value()>0?
                     static_cast<std::uint64_t>(*node.wire_count):0)==
                    node.values.size()/columns &&
                node.values.size()%columns==0,
                "BNW city tail counted span shape changed");
    }
    if(encoding=="signed_records" || encoding=="stl_records" ||
       encoding=="hashed_bool")
        require(node.wire_count && node.values.empty() &&
                (node.wire_count.value()>0?
                     static_cast<std::uint64_t>(*node.wire_count):0)==
                    node.children.size(),
                "BNW city tail record count changed");
    if(name(node.name)=="hash" || name(node.name)=="m_eReligion") {
        require(node.values.size()==1,"BNW city tail object identity shape changed");
        out.i64(node.values[0]);
    }
    for(const auto &child:node.children) node_schema(out,child,depth+1);
}
std::string schema_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(const auto &player:world.all_players->players)
        for(const auto &city:player.cities.cities) {
            const auto &tail=city.tail;
            require(tail.layout==bnw &&
                    tail.context.profile_extensions.values.empty() &&
                    tail.context.bnw_city_version==city.core.version &&
                    tail.blocks.size()==blocks.size(),
                    "Unknown BNW city tail profile or block count");
            out.u64(tail.start_offset);out.u64(tail.end_offset);
            out.u64(*tail.context.bnw_city_version);
            for(std::size_t i=0;i<blocks.size();++i) {
                require(name(tail.blocks[i].name)==blocks[i],
                        "Unknown BNW city tail block/order");
                node_schema(out,tail.blocks[i]);
            }
        }
    return identify_bytes(out.bytes).sha256;
}
void value_shape(const saves::CityTailNode &node,std::int64_t value) {
    const auto encoding=name(node.encoding);
    if(encoding=="bool" || encoding=="span_bool" || encoding=="counted_bool")
        require(value==0 || value==1,"BNW city tail boolean invalid");
    else if(encoding=="u32")
        require(value>=0 && value<=UINT32_MAX,
                "BNW city tail u32 exceeds source wire range");
    else
        require(value>=INT32_MIN && value<=INT32_MAX,
                "BNW city tail i32 exceeds source wire range");
}
template<class Node,class Fn> void visit_node(Node &node,Fn &&visit) {
    for(auto &value:node.values) visit(node,value);
    for(auto &child:node.children) visit_node(child,visit);
}
template<class World,class Fn> void walk_values(World &world,Fn &&visit) {
    for(auto &player:world.all_players->players)
        for(auto &city:player.cities.cities)
            for(auto &block:city.tail.blocks) visit_node(block,visit);
}
bool fixed_value(const saves::CityTailNode &node) {
    const auto key=name(node.name);
    return key=="version" || key=="hash" || key=="m_eReligion";
}
std::string header_hash(const saves::SavedSessionSnapshot &world) {
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
    }
    return identify_bytes(out.bytes).sha256;
}
void write_array(Writer &out,const saves::PlotArrayObservation &array) {
    out.string(name(array.name));out.u64(array.columns);
    out.u64(array.values.size());
    for(const auto value:array.values) out.i64(value);
}
void write_production(Writer &out,const saves::CityProductionRecord &record) {
    out.string(name(record.name));out.u64(record.index);
    out.u64(record.start_offset);out.u64(record.end_offset);
    out.u64(record.fields.size());
    for(const auto &field:record.fields) {
        out.string(name(field.name));out.i64(field.value);
    }
    out.u64(record.arrays.size());
    for(const auto &array:record.arrays) write_array(out,array);
    out.u64(record.blobs.size());
    for(const auto &blob:record.blobs) {
        out.string(name(blob.name));out.u64(blob.bytes.size());
        out.bytes.insert(out.bytes.end(),blob.bytes.begin(),blob.bytes.end());
    }
    out.u64(record.children.size());
    for(const auto &child:record.children) write_production(out,child);
}
std::string companion_hash(const saves::SavedSessionSnapshot &world) {
    shape(world);Writer out;
    for(const auto &player:world.all_players->players)
        for(const auto &city:player.cities.cities) {
            const auto &core=city.core;
            out.u64(core.start_offset);out.u64(core.end_offset);
            out.u32(core.version.has_value()?1:0);
            if(core.version) out.u64(*core.version);
            out.u64(core.fields.size());
            for(const auto &field:core.fields) {
                out.string(name(field.name));out.u32(static_cast<std::uint32_t>(field.encoding));
                out.u64(field.start_offset);out.u64(field.end_offset);
                out.u32(field.wire_count.has_value()?1:0);
                if(field.wire_count) out.u64(*field.wire_count);
                out.u64(field.values.size());
                for(const auto value:field.values) out.i64(value);
                out.u64(field.row_keys.size());
                for(const auto value:field.row_keys) out.i64(value);
                out.u64(field.row_counts.size());
                for(const auto value:field.row_counts) out.u64(value);
                out.u64(field.hashes.size());
                for(const auto value:field.hashes) out.u64(value);
                out.u64(field.bytes.size());
                out.bytes.insert(out.bytes.end(),field.bytes.begin(),field.bytes.end());
            }
            const auto &production=city.production;
            require(production.layout==bnw &&
                    production.context.profile_extensions.values.empty(),
                    "Unknown BNW city tail production companion profile");
            out.u64(production.start_offset);out.u64(production.end_offset);
            out.u32(production.context.bnw_city_version.has_value()?1:0);
            if(production.context.bnw_city_version)
                out.u64(*production.context.bnw_city_version);
            out.u64(production.blocks.size());
            for(const auto &block:production.blocks) write_production(out,block);
            const auto &management=city.management;
            require(management.layout==bnw &&
                    management.context.profile_extensions.values.empty(),
                    "Unknown BNW city tail management companion profile");
            out.u64(management.start_offset);out.u64(management.end_offset);
            out.u32(management.context.bnw_projects.has_value()?1:0);
            if(management.context.bnw_projects)
                out.u64(*management.context.bnw_projects);
            out.u64(management.city_plot_slots);out.u64(management.blocks.size());
            for(const auto &block:management.blocks) {
                out.string(name(block.name));out.u64(block.start_offset);
                out.u64(block.end_offset);out.u64(block.fields.size());
                for(const auto &field:block.fields) {
                    out.string(name(field.name));out.i64(field.value);
                }
                out.u64(block.arrays.size());
                for(const auto &array:block.arrays) write_array(out,array);
                out.u64(block.hashes.size());
                for(const auto &hash:block.hashes) {
                    out.string(name(hash.name));out.i64(hash.wire_count);
                    out.u64(hash.rows.size());
                    for(const auto &row:hash.rows) {
                        out.u64(row.type_hash);out.u64(row.values.size());
                        for(const auto value:row.values) out.i64(value);
                    }
                }
            }
        }
    return identify_bytes(out.bytes).sha256;
}
} // namespace

saves::CivisSaveSection make_bnw_city_tail_checkpoint(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    const auto header=header_hash(source),companions=companion_hash(source);
    require(roster_hash(world)==roster && schema_hash(world)==schema &&
            header_hash(world)==header && companion_hash(world)==companions,
            "BNW city tail roster, structure or companions changed");
    std::vector<std::int64_t> baseline;
    std::vector<bool> immutable;
    walk_values(source,[&](const auto &node,const auto &value){
        require(baseline.size()<max_values,"BNW city tail value budget exceeded");
        value_shape(node,value);baseline.push_back(value);
        immutable.push_back(fixed_value(node));
    });
    Writer out;out.bytes.insert(out.bytes.end(),magic.begin(),magic.end());
    out.u32(1);
    out.raw(identity.payload_sha256);out.raw(identity.rules_sha256);
    out.raw(roster);out.raw(schema);out.raw(header);out.raw(companions);
    out.u32(players);out.u64(baseline.size());
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const auto &node,const auto &value){
        require(index<baseline.size(),"BNW city tail value count changed");
        value_shape(node,value);
        require(!immutable[index] || value==baseline[index],
                "BNW city tail version or object identity changed");
        changed|=value!=baseline[index++];out.i64(value);
    });
    require(index==baseline.size() && (changed || allow_unchanged),
            "BNW city tail checkpoint unchanged or incomplete");
    if(!changed) out.bytes[8]=2;
    return {bnw_city_tail_checkpoint_section_id,0,std::move(out.bytes)};
}

void restore_bnw_city_tail_checkpoint(
    const saves::CivisSaveSection &section,
    saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    hash_shape(identity.payload_sha256);hash_shape(identity.rules_sha256);
    require(section.id==bnw_city_tail_checkpoint_section_id &&
            section.flags==0 && section.bytes.size()>=8+4+64*6+4+8 &&
            std::equal(magic.begin(),magic.end(),section.bytes.begin()),
            "Unsupported BNW city tail checkpoint section");
    Reader in{section.bytes,magic.size()};
    const auto version=in.u32();
    require(version==1 || version==2,
            "Unsupported BNW city tail checkpoint version");
    in.hash(identity.payload_sha256);in.hash(identity.rules_sha256);
    const auto roster=roster_hash(source),schema=schema_hash(source);
    const auto header=header_hash(source),companions=companion_hash(source);
    in.hash(roster);in.hash(schema);in.hash(header);in.hash(companions);
    require(in.u32()==players && roster_hash(world)==roster &&
            schema_hash(world)==schema && header_hash(world)==header &&
            companion_hash(world)==companions,
            "BNW city tail destination roster, structure or companions changed");
    const auto count=in.u64();
    require(count<=max_values && count<=(section.bytes.size()-in.at)/8 &&
            section.bytes.size()-in.at==count*8,
            "BNW city tail section count or length changed");
    std::vector<std::int64_t> baseline;
    baseline.reserve(static_cast<std::size_t>(count));
    std::vector<bool> immutable;
    immutable.reserve(static_cast<std::size_t>(count));
    walk_values(source,[&](const auto &node,const auto &value){
        require(baseline.size()<max_values,"BNW city tail value budget exceeded");
        value_shape(node,value);baseline.push_back(value);
        immutable.push_back(fixed_value(node));
    });
    require(baseline.size()==count,"BNW city tail source count changed");
    std::vector<std::pair<std::int64_t*,std::int64_t>> pending;
    pending.reserve(static_cast<std::size_t>(count));
    bool changed=false;std::size_t index=0;
    walk_values(world,[&](const auto &node,auto &value){
        require(index<baseline.size() && value==baseline[index],
                "BNW city tail destination is not source baseline");
        const auto next=in.i64();value_shape(node,next);
        require(!immutable[index] || next==baseline[index],
                "BNW city tail version or object identity changed");
        changed|=next!=baseline[index++];pending.emplace_back(&value,next);
    });
    require(index==count && changed==(version==1) &&
            in.at==section.bytes.size(),
            "BNW city tail section incomplete or unchanged");
    for(const auto &[target,value]:pending) *target=value;
}

BnwCityCheckpointQuadruple make_bnw_city_checkpoint_quadruple(
    const saves::SavedSessionSnapshot &world,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity,
    bool allow_unchanged) {
    require(roster_hash(world)==roster_hash(source) &&
            header_hash(world)==header_hash(source),
            "BNW city quadruple roster or collection header changed");
    auto core_only=source,production_only=source,management_only=source,
         tail_only=source;
    for(std::size_t p=0;p<players;++p) {
        const auto &from=world.all_players->players[p].cities.cities;
        auto &core=core_only.all_players->players[p].cities.cities;
        auto &production=production_only.all_players->players[p].cities.cities;
        auto &management=management_only.all_players->players[p].cities.cities;
        auto &tail=tail_only.all_players->players[p].cities.cities;
        require(from.size()==core.size() && from.size()==production.size() &&
                from.size()==management.size() && from.size()==tail.size(),
                "BNW city quadruple count changed");
        for(std::size_t i=0;i<from.size();++i) {
            core[i].core=from[i].core;
            production[i].production=from[i].production;
            management[i].management=from[i].management;
            tail[i].tail=from[i].tail;
        }
    }
    BnwCityCheckpointQuadruple result;
    result.core=make_bnw_city_core_checkpoint(
        core_only,source,identity,allow_unchanged);
    result.production=make_bnw_city_production_checkpoint(
        production_only,source,identity,allow_unchanged);
    result.management=make_bnw_city_management_checkpoint(
        management_only,source,identity,allow_unchanged);
    result.tail=make_bnw_city_tail_checkpoint(
        tail_only,source,identity,allow_unchanged);
    return result;
}

saves::SavedSessionSnapshot restore_bnw_city_checkpoint_quadruple(
    const BnwCityCheckpointQuadruple &sections,
    const saves::SavedSessionSnapshot &source,
    const SessionCheckpointIdentity &identity) {
    auto replacement=source,core_only=source,production_only=source,
         management_only=source,tail_only=source;
    restore_bnw_city_core_checkpoint(sections.core,core_only,source,identity);
    restore_bnw_city_production_checkpoint(
        sections.production,production_only,source,identity);
    restore_bnw_city_management_checkpoint(
        sections.management,management_only,source,identity);
    restore_bnw_city_tail_checkpoint(sections.tail,tail_only,source,identity);
    for(std::size_t p=0;p<players;++p) {
        auto &to=replacement.all_players->players[p].cities.cities;
        const auto &cores=core_only.all_players->players[p].cities.cities;
        const auto &productions=production_only.all_players->players[p].cities.cities;
        const auto &managements=management_only.all_players->players[p].cities.cities;
        const auto &tails=tail_only.all_players->players[p].cities.cities;
        for(std::size_t i=0;i<to.size();++i) {
            to[i].core=cores[i].core;
            to[i].production=productions[i].production;
            to[i].management=managements[i].management;
            to[i].tail=tails[i].tail;
        }
    }
    return replacement;
}
} // namespace civ5::runtime
